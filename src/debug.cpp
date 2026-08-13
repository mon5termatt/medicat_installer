#include "debug.h"

#include "drives.h"
#include "util.h"
#include "ventoy.h"

#include <windows.h>
#include <wincrypt.h>

#include <fstream>
#include <sstream>
#include <vector>

#ifndef MEDICAT_USB_VERSION
#define MEDICAT_USB_VERSION "unknown"
#endif

namespace medicat {

namespace {

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (len <= 1) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), len);
    return wide;
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

std::wstring ReadRegString(const HKEY root, const wchar_t* subkey, const wchar_t* valueName) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return {};
    }

    wchar_t buffer[512]{};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    const LSTATUS status =
        RegQueryValueExW(key, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        return {};
    }
    return buffer;
}

DWORD ReadRegDword(const HKEY root, const wchar_t* subkey, const wchar_t* valueName) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return 0;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    const LSTATUS status =
        RegQueryValueExW(key, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || type != REG_DWORD) {
        return 0;
    }
    return value;
}

struct WindowsVersionInfo {
    std::wstring productName;
    std::wstring displayVersion;
    std::wstring editionId;
    std::wstring installationType;
    DWORD ntMajor = 0;
    DWORD ntMinor = 0;
    DWORD build = 0;
    DWORD ubr = 0;
};

std::wstring DescribeEditionId(const std::wstring& editionId) {
    if (editionId.empty()) {
        return {};
    }

    struct EditionEntry {
        const wchar_t* id;
        const wchar_t* label;
    };
    static constexpr EditionEntry kEditions[] = {
        {L"Professional", L"Pro"},
        {L"ProfessionalN", L"Pro N"},
        {L"ProfessionalWorkstation", L"Pro for Workstations"},
        {L"ProfessionalWorkstationN", L"Pro for Workstations N"},
        {L"Enterprise", L"Enterprise"},
        {L"EnterpriseN", L"Enterprise N"},
        {L"EnterpriseS", L"Enterprise LTSC"},
        {L"EnterpriseSN", L"Enterprise LTSC N"},
        {L"IoTEnterprise", L"IoT Enterprise"},
        {L"IoTEnterpriseS", L"IoT Enterprise LTSC"},
        {L"Core", L"Home"},
        {L"CoreN", L"Home N"},
        {L"CoreSingleLanguage", L"Home Single Language"},
        {L"CoreCountrySpecific", L"Home China"},
        {L"Education", L"Education"},
        {L"EducationN", L"Education N"},
        {L"ServerStandard", L"Server Standard"},
        {L"ServerDatacenter", L"Server Datacenter"},
        {L"ServerDatacenterCore", L"Server Datacenter Core"},
    };

    for (const EditionEntry& entry : kEditions) {
        if (_wcsicmp(editionId.c_str(), entry.id) == 0) {
            return entry.label;
        }
    }
    return editionId;
}

WindowsVersionInfo GetWindowsVersionInfo() {
    constexpr wchar_t kNtVersionKey[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";

    WindowsVersionInfo info;
    GetOsVersion(info.ntMajor, info.ntMinor, info.build);

    info.productName = ReadRegString(HKEY_LOCAL_MACHINE, kNtVersionKey, L"ProductName");
    info.displayVersion = ReadRegString(HKEY_LOCAL_MACHINE, kNtVersionKey, L"DisplayVersion");
    if (info.displayVersion.empty()) {
        info.displayVersion = ReadRegString(HKEY_LOCAL_MACHINE, kNtVersionKey, L"ReleaseId");
    }
    info.editionId = ReadRegString(HKEY_LOCAL_MACHINE, kNtVersionKey, L"EditionID");
    info.installationType = ReadRegString(HKEY_LOCAL_MACHINE, kNtVersionKey, L"InstallationType");

    const std::wstring regBuild = ReadRegString(HKEY_LOCAL_MACHINE, kNtVersionKey, L"CurrentBuildNumber");
    if (!regBuild.empty()) {
        info.build = static_cast<DWORD>(std::wcstoul(regBuild.c_str(), nullptr, 10));
    }
    info.ubr = ReadRegDword(HKEY_LOCAL_MACHINE, kNtVersionKey, L"UBR");

    if (info.build >= 22000 && info.productName.rfind(L"Windows 10", 0) == 0) {
        info.productName.replace(0, 10, L"Windows 11");
    }

    return info;
}

std::wstring FormatWindowsVersionLine(const WindowsVersionInfo& info) {
    std::wostringstream ss;
    if (!info.productName.empty()) {
        ss << info.productName;
    } else if (info.ntMajor != 0 || info.ntMinor != 0) {
        ss << info.ntMajor << L"." << info.ntMinor;
    } else {
        ss << L"Windows";
    }

    if (!info.displayVersion.empty()) {
        ss << L", version " << info.displayVersion;
    }

    ss << L", OS build " << info.build;
    if (info.ubr > 0) {
        ss << L"." << info.ubr;
    }
    return ss.str();
}

std::wstring FormatNtVersionLine(const WindowsVersionInfo& info) {
    std::wostringstream ss;
    ss << info.ntMajor << L"." << info.ntMinor << L"." << info.build;
    if (info.ubr > 0) {
        ss << L"." << info.ubr;
    }
    return ss.str();
}

std::wstring GetProcessorName() {
    constexpr wchar_t kCpuKey[] = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
    std::wstring name = ReadRegString(HKEY_LOCAL_MACHINE, kCpuKey, L"ProcessorNameString");
    while (!name.empty() && (name.front() == L' ' || name.front() == L'\t')) {
        name.erase(name.begin());
    }
    while (!name.empty() && (name.back() == L' ' || name.back() == L'\t')) {
        name.pop_back();
    }
    return name;
}

std::wstring GetSystemModelLine() {
    constexpr wchar_t kBiosKey[] = L"HARDWARE\\DESCRIPTION\\System\\BIOS";
    const std::wstring manufacturer = ReadRegString(HKEY_LOCAL_MACHINE, kBiosKey, L"SystemManufacturer");
    const std::wstring product = ReadRegString(HKEY_LOCAL_MACHINE, kBiosKey, L"SystemProductName");
    if (manufacturer.empty() && product.empty()) {
        return {};
    }
    if (manufacturer.empty()) {
        return product;
    }
    if (product.empty()) {
        return manufacturer;
    }
    return manufacturer + L" " + product;
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
    return Utf8ToWide(content);
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

std::string TrimLineWhitespace(std::string line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
    }
    size_t start = 0;
    while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }
    return line.substr(start);
}

std::string ExtractToolVersionLine(const std::string& captured) {
    std::string firstNonEmpty;
    size_t pos = 0;
    while (pos < captured.size()) {
        size_t end = captured.find('\n', pos);
        if (end == std::string::npos) {
            end = captured.size();
        }
        std::string line = TrimLineWhitespace(captured.substr(pos, end - pos));
        pos = (end < captured.size()) ? end + 1 : captured.size();
        if (line.empty()) {
            continue;
        }
        if (firstNonEmpty.empty()) {
            firstNonEmpty = line;
        }
        if (line.rfind("7-Zip", 0) == 0 || line.rfind("aria2 version", 0) == 0) {
            return line;
        }
    }
    return firstNonEmpty;
}

std::wstring GetToolVersionLine(const std::wstring& exePath, const wchar_t* extraArgs = L"-h") {
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

    HANDLE nullIn = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.hStdInput = (nullIn != INVALID_HANDLE_VALUE) ? nullIn : nullptr;

    std::wstring cmd = L"\"" + exePath + L"\" ";
    cmd += extraArgs && extraArgs[0] != L'\0' ? extraArgs : L"-h";
    std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back(L'\0');

    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmdBuffer.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si,
                        &pi)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);
        if (nullIn != INVALID_HANDLE_VALUE) {
            CloseHandle(nullIn);
        }
        return L"(could not launch)";
    }

    if (nullIn != INVALID_HANDLE_VALUE) {
        CloseHandle(nullIn);
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    std::string captured;
    char buffer[512];
    auto drainAvailable = [&](HANDLE pipe) {
        for (;;) {
            DWORD available = 0;
            if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr) || available == 0) {
                break;
            }
            const DWORD toRead = (available < sizeof(buffer)) ? available : static_cast<DWORD>(sizeof(buffer));
            DWORD read = 0;
            if (!ReadFile(pipe, buffer, toRead, &read, nullptr) || read == 0) {
                break;
            }
            captured.append(buffer, read);
            if (captured.size() > 4096) {
                break;
            }
        }
    };

    const DWORD startTick = GetTickCount();
    for (;;) {
        drainAvailable(stdoutRead);
        drainAvailable(stderrRead);

        const DWORD wait = WaitForSingleObject(pi.hProcess, 50);
        if (wait == WAIT_OBJECT_0) {
            break;
        }
        if (GetTickCount() - startTick > 5000) {
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 1000);
            break;
        }
    }

    drainAvailable(stdoutRead);
    drainAvailable(stderrRead);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);

    const std::string versionLine = ExtractToolVersionLine(captured);
    if (versionLine.empty()) {
        return L"(no version output)";
    }
    return Utf8ToWide(versionLine);
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

void WriteApplicationSection(const DiagnosticContext& context,
                             const std::function<void(const std::wstring&)>& write) {
    auto field = [&](const wchar_t* label, const std::wstring& value) {
        write(std::wstring(label) + value);
    };

    write(kDiagnosticSeparator);
    write(L"[Application]");
    field(L"Installer version: ", InstallerVersionLabel());
    field(L"MediCat USB version: ", Utf8ToWide(MEDICAT_USB_VERSION));
    field(L"Executable directory: ", context.outputDir);
    field(L"Process architecture: ", GetProcessorArchitecture());
    field(L"Elevated admin: ", IsProcessElevated() ? L"yes" : L"no");
}

void WriteSystemSection(const DiagnosticContext& context, const std::function<void(const std::wstring&)>& write) {
    auto field = [&](const wchar_t* label, const std::wstring& value) {
        write(std::wstring(label) + value);
    };
    (void)context;

    write(kDiagnosticSeparator);
    write(L"[System]");

    const WindowsVersionInfo win = GetWindowsVersionInfo();
    if (win.ntMajor != 0 || win.build != 0 || !win.productName.empty()) {
        field(L"Windows: ", FormatWindowsVersionLine(win));
        field(L"NT version: ", FormatNtVersionLine(win));
        const std::wstring edition = DescribeEditionId(win.editionId);
        if (!edition.empty()) {
            field(L"Edition: ", edition);
            if (!win.editionId.empty() && _wcsicmp(win.editionId.c_str(), edition.c_str()) != 0) {
                field(L"Edition ID: ", win.editionId);
            }
        }
        if (!win.installationType.empty()) {
            field(L"Installation type: ", win.installationType);
        }
    } else {
        field(L"Windows: ", L"(unknown)");
    }

    const std::wstring systemModel = GetSystemModelLine();
    if (!systemModel.empty()) {
        field(L"System: ", systemModel);
    }

    const std::wstring cpuName = GetProcessorName();
    if (!cpuName.empty()) {
        field(L"Processor: ", cpuName);
    }
    field(L"Processor architecture: ", GetProcessorArchitecture());

    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD computerLen = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(computerName, &computerLen)) {
        field(L"Computer name: ", std::wstring(computerName, computerLen));
    }

    wchar_t userName[256]{};
    DWORD userLen = static_cast<DWORD>(std::size(userName));
    if (GetUserNameW(userName, &userLen)) {
        field(L"User name: ", std::wstring(userName, userLen));
    }

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0) {
        field(L"User locale: ", localeName);
    }

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        std::wostringstream ram;
        ram << FormatBytes(mem.ullTotalPhys) << L" total, " << FormatBytes(mem.ullAvailPhys) << L" available";
        field(L"Physical memory: ", ram.str());
    }

    SYSTEM_INFO sys{};
    GetSystemInfo(&sys);
    field(L"Logical processors: ", std::to_wstring(sys.dwNumberOfProcessors));
}

void WriteBundledToolsSection(const DiagnosticContext& context,
                              const std::function<void(const std::wstring&)>& write,
                              const std::function<void(const std::wstring&)>& writeDebug) {
    auto field = [&](const wchar_t* label, const std::wstring& value) {
        write(std::wstring(label) + value);
    };

    write(kDiagnosticSeparator);
    write(L"[Bundled tools]");
    field(L"7za.exe: ", DescribeFile(context.sevenZaPath));
    field(L"7za version: ", GetToolVersionLine(context.sevenZaPath));
    field(L"aria2c.exe: ", DescribeFile(context.aria2cPath));
    field(L"aria2c version: ", GetToolVersionLine(context.aria2cPath, L"--version"));
    field(L"MedicatFiles.md5: ", DescribeFile(context.md5ManifestPath));
    field(L"MediCat archive: ", DescribeFile(context.archivePath));
    if (writeDebug) {
        const std::wstring archiveDebug = BuildMediCatArchiveSizeDebugLine(context.archivePath);
        if (!archiveDebug.empty()) {
            writeDebug(archiveDebug);
        }
    }
    write(kDiagnosticSeparator);
}

void WriteInstallerOptionsSection(const DiagnosticContext& context,
                                  const std::function<void(const std::wstring&)>& write) {
    auto field = [&](const wchar_t* label, const std::wstring& value) {
        write(std::wstring(label) + value);
    };

    write(kDiagnosticSeparator);
    write(L"[Installer options]");
    if (!context.operation.empty()) {
        field(L"Operation: ", context.operation);
    }
    field(L"Selected drive: ", DescribeDrive(context.selectedDrive));
    field(L"Format drive: ", context.formatChecked ? L"yes" : L"no");
    field(L"Run Ventoy: ", context.runVentoyChecked ? L"yes" : L"no");
    field(L"Ventoy GPT: ", context.ventoyGpt ? L"yes" : L"no");
    field(L"Ventoy Secure Boot: ", context.ventoySecureBoot ? L"enabled" : L"disabled");
    field(L"Pinned Ventoy version: ",
          context.pinnedVentoyVersion.empty() ? L"(latest)" : context.pinnedVentoyVersion);
    write(kDiagnosticSeparator);
}

std::string Sha256HexUtf8(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return {};
    }
    if (!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) {
        CryptReleaseContext(provider, 0);
        return {};
    }

    const auto cleanup = [&]() {
        if (hash) {
            CryptDestroyHash(hash);
        }
        if (provider) {
            CryptReleaseContext(provider, 0);
        }
    };

    if (!CryptHashData(hash, reinterpret_cast<const BYTE*>(input.data()), static_cast<DWORD>(input.size()), 0)) {
        cleanup();
        return {};
    }

    BYTE digest[32]{};
    DWORD digestLen = sizeof(digest);
    if (!CryptGetHashParam(hash, HP_HASHVAL, digest, &digestLen, 0)) {
        cleanup();
        return {};
    }
    cleanup();

    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(64, '\0');
    for (DWORD i = 0; i < digestLen; ++i) {
        out[i * 2] = kHex[(digest[i] >> 4) & 0x0F];
        out[i * 2 + 1] = kHex[digest[i] & 0x0F];
    }
    return out;
}

std::string ReadMachineGuidUtf8() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY,
                      &key) != ERROR_SUCCESS) {
        return {};
    }

    wchar_t buffer[64]{};
    DWORD size = sizeof(buffer);
    const LSTATUS status =
        RegQueryValueExW(key, L"MachineGuid", nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || buffer[0] == L'\0') {
        return {};
    }

    std::string guid;
    const int len = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
    if (len > 1) {
        guid.assign(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, buffer, -1, guid.data(), len, nullptr, nullptr);
    }
    return guid;
}

std::string GetMachineIdHashUtf8() {
    std::string guid = ReadMachineGuidUtf8();
    if (guid.empty()) {
        return {};
    }
    for (char& ch : guid) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return Sha256HexUtf8(guid);
}

SessionSystemSnapshot BuildSessionSystemSnapshot() {
    SessionSystemSnapshot snapshot;
    const WindowsVersionInfo win = GetWindowsVersionInfo();
    snapshot.windowsBuild = static_cast<int>(win.build);
    snapshot.windowsUbr = static_cast<int>(win.ubr);
    if (win.ntMajor != 0 || win.ntMinor != 0) {
        snapshot.windowsMajorMinor = std::to_string(win.ntMajor) + "." + std::to_string(win.ntMinor);
    }

    auto wideToUtf8 = [](const std::wstring& text) {
        if (text.empty()) {
            return std::string();
        }
        const int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1) {
            return std::string();
        }
        std::string out(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), len, nullptr, nullptr);
        return out;
    };

    snapshot.editionId = wideToUtf8(win.editionId);
    snapshot.installationType = wideToUtf8(win.installationType);
    snapshot.processorArch = wideToUtf8(GetProcessorArchitecture());

    SYSTEM_INFO sys{};
    GetSystemInfo(&sys);
    snapshot.logicalProcessors = static_cast<int>(sys.dwNumberOfProcessors);

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        const uint64_t gb = mem.ullTotalPhys / (1024ULL * 1024ULL * 1024ULL);
        if (gb >= 64) {
            snapshot.ramGbBucket = "64+";
        } else if (gb >= 32) {
            snapshot.ramGbBucket = "32";
        } else if (gb >= 16) {
            snapshot.ramGbBucket = "16";
        } else if (gb >= 8) {
            snapshot.ramGbBucket = "8";
        } else {
            snapshot.ramGbBucket = "4";
        }
    }

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0) {
        snapshot.locale = wideToUtf8(localeName);
    }

    snapshot.machineIdHash = GetMachineIdHashUtf8();

    return snapshot;
}

}  // namespace

std::wstring BuildMediCatArchiveSizeDebugLine(const std::wstring& path) {
    if (path.empty()) {
        return {};
    }
    if (!FileExists(path)) {
        // Still note missing extract path (e.g. .zip.001 while other volumes exist).
        return L"MediCat archive: " + path + L" — missing";
    }
    const uint64_t size = GetFileSizeBytes(path);
    return L"MediCat archive: " + path + L" — " + FormatBytes(size) + L" (" + std::to_wstring(size) +
           L" bytes)";
}

void LogSystemDiagnostics(const DiagnosticContext& context,
                          const std::function<void(const std::wstring&)>& logLine,
                          const std::function<void(const std::wstring&)>& logDebug) {
    WriteApplicationSection(context, logLine);
    WriteSystemSection(context, logLine);
    WriteBundledToolsSection(context, logLine, logDebug);
}

void LogInstallerDiagnostics(const DiagnosticContext& context,
                             const std::function<void(const std::wstring&)>& logLine) {
    WriteInstallerOptionsSection(context, logLine);
}

SessionSystemSnapshot CollectSessionSystemSnapshot() {
    return BuildSessionSystemSnapshot();
}

}  // namespace medicat
