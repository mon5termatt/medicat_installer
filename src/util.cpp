#include "util.h"

#include <shlwapi.h>
#include <windows.h>

#include <filesystem>
#include <sstream>

namespace medicat {

std::wstring GetExeDirectory() {
    wchar_t buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return L".";
    }
    std::wstring path(buf, n);
    const auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, pos);
}

std::wstring GetMedicatTempRoot() {
    wchar_t temp[MAX_PATH]{};
    const DWORD len = GetTempPathW(MAX_PATH, temp);
    if (len == 0 || len >= MAX_PATH) {
        return JoinPath(GetExeDirectory(), L"MedicatInstaller");
    }
    return JoinPath(std::wstring(temp, len), L"MedicatInstaller");
}

namespace {

bool IsProcessRunning(const DWORD pid) {
    if (pid == 0) {
        return false;
    }

    const HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!process) {
        return false;
    }

    const DWORD wait = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return wait == WAIT_TIMEOUT;
}

DWORD ParsePidDirName(const std::wstring& name) {
    if (name.empty()) {
        return 0;
    }

    for (const wchar_t ch : name) {
        if (ch < L'0' || ch > L'9') {
            return 0;
        }
    }

    return static_cast<DWORD>(std::wcstoul(name.c_str(), nullptr, 10));
}

}  // namespace

std::wstring GetMedicatTempDir() {
    const std::wstring root = GetMedicatTempRoot();
    CreateDirectoryW(root.c_str(), nullptr);

    const std::wstring dir = JoinPath(root, std::to_wstring(GetCurrentProcessId()));
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

void CleanupMedicatTempOnExit() {
    namespace fs = std::filesystem;

    const std::wstring root = GetMedicatTempRoot();
    if (root.empty()) {
        return;
    }

    std::error_code ec;
    if (!fs::exists(root, ec) || ec) {
        return;
    }

    const DWORD currentPid = GetCurrentProcessId();
    for (const fs::directory_entry& entry : fs::directory_iterator(root, ec)) {
        if (ec) {
            break;
        }

        if (!entry.is_directory(ec)) {
            fs::remove(entry.path(), ec);
            ec.clear();
            continue;
        }

        const DWORD pid = ParsePidDirName(entry.path().filename().wstring());
        if (pid == 0) {
            continue;
        }

        if (pid == currentPid || !IsProcessRunning(pid)) {
            fs::remove_all(entry.path(), ec);
            ec.clear();
        }
    }

    if (fs::is_empty(root, ec)) {
        fs::remove(root, ec);
    }
}

std::wstring JoinPath(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) {
        return b;
    }
    wchar_t out[MAX_PATH]{};
    if (PathCombineW(out, a.c_str(), b.c_str())) {
        return out;
    }
    if (a.back() == L'\\' || a.back() == L'/') {
        return a + b;
    }
    return a + L'\\' + b;
}

bool FileExists(const std::wstring& path) {
    const DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

uint64_t GetFileSizeBytes(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA info{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info)) {
        return 0;
    }
    if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return 0;
    }
    return (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
}

std::wstring FormatBytes(uint64_t bytes) {
    const double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gb >= 1.0) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(0);
        ss << gb << L" GB";
        return ss.str();
    }
    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    std::wostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(1);
    ss << mb << L" MB";
    return ss.str();
}

std::wstring FormatProgressBytes(const uint64_t bytes) {
    const double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gb >= 1.0) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(2);
        ss << gb << L" GB";
        return ss.str();
    }

    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    if (mb >= 1.0) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(1);
        ss << mb << L" MB";
        return ss.str();
    }

    const double kb = static_cast<double>(bytes) / 1024.0;
    if (kb >= 1.0) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(0);
        ss << kb << L" KB";
        return ss.str();
    }

    return std::to_wstring(bytes) + L" B";
}

std::wstring FormatDownloadSpeed(const uint64_t bytesPerSecond) {
    if (bytesPerSecond == 0) {
        return L"";
    }

    const double gbPerSecond = static_cast<double>(bytesPerSecond) / (1024.0 * 1024.0 * 1024.0);
    if (gbPerSecond >= 1.0) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(2);
        ss << gbPerSecond << L" GB/s";
        return ss.str();
    }

    const double mbPerSecond = static_cast<double>(bytesPerSecond) / (1024.0 * 1024.0);
    if (mbPerSecond >= 0.1) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(1);
        ss << mbPerSecond << L" MB/s";
        return ss.str();
    }

    const double kbPerSecond = static_cast<double>(bytesPerSecond) / 1024.0;
    if (kbPerSecond >= 1.0) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(0);
        ss << kbPerSecond << L" KB/s";
        return ss.str();
    }

    return std::to_wstring(bytesPerSecond) + L" B/s";
}

std::wstring FormatPercent(int percent) {
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    return std::to_wstring(percent) + L"%";
}

std::wstring ShortDisplayPath(const std::wstring& path, const size_t maxLen) {
    if (path.size() <= maxLen) {
        return path;
    }
    return L"..." + path.substr(path.size() - (maxLen - 3));
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    std::wstring line;
    for (wchar_t ch : text) {
        if (ch == L'\n' || ch == L'\r') {
            if (!line.empty()) {
                lines.push_back(line);
                line.clear();
            }
        } else if (ch != L'\b') {
            line.push_back(ch);
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return lines;
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

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) {
        return {};
    }
    std::string out(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), len, nullptr, nullptr);
    return out;
}

std::wstring InstallerVersionWide() {
    if (kInstallerVersion[0] == '\0') {
        return L"0.0.0";
    }
    return Utf8ToWide(kInstallerVersion);
}

std::wstring InstallerVersionLabel() {
    const std::wstring version = InstallerVersionWide();
    if (version.empty()) {
        return L"v0.0.0";
    }
    if (version.front() == L'v' || version.front() == L'V') {
        return version;
    }
    return L"v" + version;
}

}  // namespace medicat
