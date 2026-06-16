#include "debug.h"

#include "drives.h"
#include "util.h"
#include "ventoy.h"

#include <windows.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#ifndef INSTALLER_VERSION
#define INSTALLER_VERSION "dev"
#endif
#ifndef MEDICAT_USB_VERSION
#define MEDICAT_USB_VERSION "unknown"
#endif

namespace medicat {

namespace {

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }
    std::string out(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

std::wstring NowStamp() {
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::wostringstream ss;
    ss << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void AppendLine(std::ostringstream& out, const char* label, const std::wstring& value) {
    out << label << WideToUtf8(value) << "\n";
}

void AppendLineUtf8(std::ostringstream& out, const char* label, const std::string& value) {
    out << label << value << "\n";
}

bool IsProcessElevated() {
    BOOL elevated = FALSE;
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
        elevated = elevation.TokenIsElevated;
    }
    CloseHandle(token);
    return elevated != FALSE;
}

bool GetOsVersion(DWORD& major, DWORD& minor, DWORD& build) {
    using RtlGetVersionPtr = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return false;
    }
    const auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (!rtlGetVersion) {
        return false;
    }

    RTL_OSVERSIONINFOW info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if (rtlGetVersion(&info) != 0) {
        return false;
    }

    major = info.dwMajorVersion;
    minor = info.dwMinorVersion;
    build = info.dwBuildNumber;
    return true;
}

std::wstring GetProcessorArchitecture() {
    USHORT processMachine = 0;
    USHORT nativeMachine = 0;
    if (IsWow64Process2(GetCurrentProcess(), &processMachine, &nativeMachine)) {
        switch (nativeMachine) {
            case IMAGE_FILE_MACHINE_AMD64:
                return L"x64";
            case IMAGE_FILE_MACHINE_ARM64:
                return L"ARM64";
            case IMAGE_FILE_MACHINE_I386:
                return L"x86";
            default:
                break;
        }
    }

#if defined(_M_ARM64)
    return L"ARM64";
#elif defined(_M_X64)
    return L"x64";
#else
    return L"x86";
#endif
}

std::wstring QueryEnvVar(const wchar_t* name) {
    wchar_t buffer[32767]{};
    const DWORD len = GetEnvironmentVariableW(name, buffer, static_cast<DWORD>(std::size(buffer)));
    if (len == 0 || len >= std::size(buffer)) {
        return L"";
    }
    return std::wstring(buffer, len);
}

std::wstring ReadDriveVentoyVersion(const std::wstring& driveLetter) {
    if (driveLetter.size() < 2) {
        return L"";
    }
    std::wstring root = driveLetter;
    if (root.back() != L'\\') {
        root += L'\\';
    }
    const std::wstring versionFile = JoinPath(JoinPath(root, L"ventoy"), L"version");
    if (!FileExists(versionFile)) {
        return L"";
    }

    std::ifstream in(versionFile, std::ios::binary);
    if (!in) {
        return L"";
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    while (!content.empty() && (content.back() == '\r' || content.back() == '\n' || content.back() == ' ')) {
        content.pop_back();
    }
    if (content.empty()) {
        return L"";
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
    if (len <= 1) {
        return L"";
    }
    std::wstring wide(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, wide.data(), len);
    return wide;
}

std::wstring DescribeDrive(const std::wstring& driveLetter) {
    if (driveLetter.empty()) {
        return L"(none selected)";
    }

    std::wstring root = driveLetter;
    if (root.size() == 2 && root[1] == L':') {
        root += L'\\';
    }

    wchar_t label[MAX_PATH + 1]{};
    wchar_t fs[MAX_PATH + 1]{};
    DWORD serial = 0;
    DWORD maxComp = 0;
    DWORD flags = 0;
    if (!GetVolumeInformationW(root.c_str(), label, MAX_PATH, &serial, &maxComp, &flags, fs, MAX_PATH)) {
        return driveLetter + L" (volume info unavailable)";
    }

    const uint64_t total = GetDriveTotalBytes(driveLetter);
    const uint64_t free = GetDriveFreeBytes(root);
    const bool ventoy = TestVentoyInstalled(driveLetter);
    const std::wstring ventoyVersion = ReadDriveVentoyVersion(driveLetter);

    std::wostringstream ss;
    ss << driveLetter << L"  label=\"" << label << L"\"  fs=" << fs << L"  total=" << FormatBytes(total)
       << L"  free=" << FormatBytes(free) << L"  ventoy=" << (ventoy ? L"yes" : L"no");
    if (!ventoyVersion.empty()) {
        ss << L"  ventoy_version=" << ventoyVersion;
    }
    return ss.str();
}

std::wstring GetToolVersionLine(const std::wstring& exePath) {
    if (exePath.empty() || !FileExists(exePath)) {
        return L"(not found)";
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    HANDLE stderrRead = nullptr;
    HANDLE stderrWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0) || !CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        return L"(could not create pipe)";
    }

    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    std::wstring cmd = L"\"" + exePath + L"\"";
    std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back(L'\0');

    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmdBuffer.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si,
                        &pi)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);
        return L"(could not launch)";
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    std::string captured;
    char buffer[512];
    auto drain = [&](HANDLE pipe) {
        for (;;) {
            DWORD read = 0;
            if (!ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr) || read == 0) {
                break;
            }
            captured.append(buffer, read);
            if (captured.size() > 4096) {
                break;
            }
        }
    };

    const DWORD wait = WaitForSingleObject(pi.hProcess, 5000);
    drain(stdoutRead);
    drain(stderrRead);
    if (wait == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);

    const auto firstLine = [&]() -> std::string {
        size_t end = captured.find('\n');
        std::string line = captured.substr(0, end == std::string::npos ? captured.size() : end);
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        return line;
    }();

    if (firstLine.empty()) {
        return L"(no version output)";
    }

    const int len = MultiByteToWideChar(CP_UTF8, 0, firstLine.c_str(), -1, nullptr, 0);
    if (len <= 1) {
        return L"(invalid version output)";
    }
    std::wstring wide(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, firstLine.c_str(), -1, wide.data(), len);
    return wide;
}

std::wstring DescribeFile(const std::wstring& path) {
    if (path.empty()) {
        return L"(not set)";
    }
    if (!FileExists(path)) {
        return path + L" (missing)";
    }

    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        return path + L" (exists)";
    }

    const uint64_t size =
        (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | static_cast<uint64_t>(data.nFileSizeLow);
    return path + L" (" + FormatBytes(size) + L")";
}

std::vector<std::string> ReadInstallerLogTail(const std::wstring& path, const size_t maxLines) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
        if (lines.size() > maxLines) {
            lines.erase(lines.begin());
        }
    }
    return lines;
}

}  // namespace

std::wstring WriteDebugLog(const DebugReportContext& context) {
    if (context.outputDir.empty()) {
        return L"";
    }

    const std::wstring outputPath = JoinPath(context.outputDir, L"debug.log");
    std::ostringstream body;
    body << "===== MediCat Installer debug.log =====\n";
    AppendLineUtf8(body, "Generated: ", WideToUtf8(NowStamp()));
    AppendLine(body, "Operation: ", context.operation.empty() ? L"unknown" : context.operation);
    AppendLine(body, "Error title: ", context.errorTitle);
    AppendLine(body, "Error message: ", context.errorMessage);
    body << "\n";

    body << "[Application]\n";
    AppendLineUtf8(body, "Installer version: ", INSTALLER_VERSION);
    AppendLineUtf8(body, "MediCat USB version: ", MEDICAT_USB_VERSION);
    AppendLine(body, "Executable directory: ", context.outputDir);
    AppendLine(body, "Process architecture: ", GetProcessorArchitecture());
    AppendLine(body, "Elevated admin: ", IsProcessElevated() ? L"yes" : L"no");
    body << "\n";

    body << "[System]\n";
    DWORD major = 0;
    DWORD minor = 0;
    DWORD build = 0;
    if (GetOsVersion(major, minor, build)) {
        std::wostringstream os;
        os << major << L"." << minor << L" (build " << build << L")";
        AppendLine(body, "Windows version: ", os.str());
    } else {
        AppendLine(body, "Windows version: ", L"(unknown)");
    }

    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD computerLen = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(computerName, &computerLen)) {
        AppendLine(body, "Computer name: ", std::wstring(computerName, computerLen));
    }

    wchar_t userName[256]{};
    DWORD userLen = static_cast<DWORD>(std::size(userName));
    if (GetUserNameW(userName, &userLen)) {
        AppendLine(body, "User name: ", std::wstring(userName, userLen));
    }

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0) {
        AppendLine(body, "User locale: ", localeName);
    }

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        const uint64_t totalRam = mem.ullTotalPhys;
        const uint64_t availRam = mem.ullAvailPhys;
        std::wostringstream ram;
        ram << FormatBytes(totalRam) << L" total, " << FormatBytes(availRam) << L" available";
        AppendLine(body, "Physical memory: ", ram.str());
    }

    SYSTEM_INFO sys{};
    GetSystemInfo(&sys);
    body << "Logical processors: " << sys.dwNumberOfProcessors << "\n";
    AppendLine(body, "PROCESSOR_ARCHITECTURE env: ", QueryEnvVar(L"PROCESSOR_ARCHITECTURE"));
    AppendLine(body, "PROCESSOR_IDENTIFIER env: ", QueryEnvVar(L"PROCESSOR_IDENTIFIER"));
    body << "\n";

    body << "[Installer options]\n";
    AppendLine(body, "Selected drive: ", DescribeDrive(context.selectedDrive));
    AppendLine(body, "Format drive: ", context.formatChecked ? L"yes" : L"no");
    AppendLine(body, "Run Ventoy: ", context.runVentoyChecked ? L"yes" : L"no");
    AppendLine(body, "Ventoy GPT: ", context.ventoyGpt ? L"yes" : L"no");
    AppendLine(body, "Ventoy Secure Boot: ", context.ventoySecureBoot ? L"enabled" : L"disabled");
    AppendLine(body, "Pinned Ventoy version: ",
               context.pinnedVentoyVersion.empty() ? L"(latest)" : context.pinnedVentoyVersion);
    body << "\n";

    body << "[Bundled tools]\n";
    AppendLine(body, "7za.exe: ", DescribeFile(context.sevenZaPath));
    AppendLine(body, "7za version: ", GetToolVersionLine(context.sevenZaPath));
    AppendLine(body, "7z.exe: ", DescribeFile(context.sevenZPath));
    AppendLine(body, "7z version: ", GetToolVersionLine(context.sevenZPath));
    AppendLine(body, "MedicatFiles.md5: ", DescribeFile(context.md5ManifestPath));
    AppendLine(body, "MediCat archive: ", DescribeFile(context.archivePath));
    body << "\n";

    body << "[Recent installer log]\n";
    const std::vector<std::string> tail = ReadInstallerLogTail(context.installerLogPath, 80);
    if (tail.empty()) {
        body << "(installer log unavailable)\n";
    } else {
        for (const std::string& line : tail) {
            body << line << "\n";
        }
    }

    std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        return L"";
    }
    const std::string content = body.str();
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out.good()) {
        return L"";
    }
    return outputPath;
}

}  // namespace medicat
