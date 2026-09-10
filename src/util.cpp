#include "util.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <windows.h>

#include <algorithm>
#include <cwchar>
#include <filesystem>
#include <sstream>
#include <vector>

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

std::wstring GetLogsDirectory() {
    return JoinPath(GetExeDirectory(), L"logs");
}

std::wstring GetLogFilePath(const std::wstring& fileName) {
    return JoinPath(GetLogsDirectory(), fileName);
}

namespace {

constexpr size_t kMaxArchivedLogSessions = 10;

// Session files written under logs/ (rotated together into archive/<timestamp>/).
const wchar_t* const kSessionLogFiles[] = {
    L"medicat_installer.log", L"ventoy.log", L"extract.log",     L"reextract.log",
    L"check.log",             L"aria.log",   L"failed_files.txt", L"cli_log.txt",
    L"cli_done.txt",          L"cli_percent.txt",
};

std::wstring FormatSessionArchiveFolderName() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t name[64]{};
    swprintf_s(name, L"%04u-%02u-%02u_%02u%02u%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
               st.wSecond);
    return name;
}

bool IsTimestampArchiveFolderName(const std::wstring& name) {
    // YYYY-MM-DD_HHMMSS
    if (name.size() != 17) {
        return false;
    }
    for (size_t i = 0; i < name.size(); ++i) {
        const wchar_t ch = name[i];
        if (i == 4 || i == 7) {
            if (ch != L'-') {
                return false;
            }
        } else if (i == 10) {
            if (ch != L'_') {
                return false;
            }
        } else if (ch < L'0' || ch > L'9') {
            return false;
        }
    }
    return true;
}

void PruneLogSessionArchives(const std::wstring& archiveDir) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(archiveDir, ec) || ec) {
        return;
    }

    std::vector<fs::path> sessions;
    for (const fs::directory_entry& entry : fs::directory_iterator(archiveDir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_directory(ec)) {
            continue;
        }
        const std::wstring name = entry.path().filename().wstring();
        if (IsTimestampArchiveFolderName(name)) {
            sessions.push_back(entry.path());
        }
    }

    if (sessions.size() <= kMaxArchivedLogSessions) {
        return;
    }

    std::sort(sessions.begin(), sessions.end());
    const size_t removeCount = sessions.size() - kMaxArchivedLogSessions;
    for (size_t i = 0; i < removeCount; ++i) {
        fs::remove_all(sessions[i], ec);
        ec.clear();
    }
}

void MigrateLegacyLogFile(const std::wstring& exeDir, const std::wstring& logsDir, const std::wstring& archiveDir,
                          const wchar_t* fileName) {
    const std::wstring legacyPath = JoinPath(exeDir, fileName);
    if (!FileExists(legacyPath)) {
        return;
    }

    const std::wstring targetPath = JoinPath(logsDir, fileName);
    if (!FileExists(targetPath)) {
        MoveFileW(legacyPath.c_str(), targetPath.c_str());
        return;
    }

    // Both exist: park the root copy under archive so logs/ stays authoritative.
    const std::wstring legacyDir = JoinPath(archiveDir, L"legacy_root");
    CreateDirectoryW(legacyDir.c_str(), nullptr);
    const std::wstring archivePath = JoinPath(legacyDir, fileName);
    if (!MoveFileW(legacyPath.c_str(), archivePath.c_str())) {
        DeleteFileW(legacyPath.c_str());
    }
}

bool GetFileLastWriteTime(const std::wstring& path, FILETIME& outWrite) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }
    outWrite = data.ftLastWriteTime;
    return true;
}

bool FileTimeNewer(const FILETIME& a, const FILETIME& b) {
    return CompareFileTime(&a, &b) > 0;
}

FILETIME NewestKnownCliLogWriteTime(const std::wstring& logsDir, const std::wstring& archiveDir) {
    FILETIME newest{};
    bool have = false;

    auto consider = [&](const std::wstring& path) {
        FILETIME write{};
        if (!GetFileLastWriteTime(path, write)) {
            return;
        }
        if (!have || FileTimeNewer(write, newest)) {
            newest = write;
            have = true;
        }
    };

    consider(JoinPath(logsDir, L"cli_log.txt"));

    namespace fs = std::filesystem;
    std::error_code ec;
    if (fs::exists(archiveDir, ec) && !ec) {
        for (const fs::directory_entry& entry : fs::directory_iterator(archiveDir, ec)) {
            if (ec || !entry.is_directory(ec)) {
                continue;
            }
            consider(JoinPath(entry.path().wstring(), L"cli_log.txt"));
        }
    }

    return newest;  // zeroed if none known
}

void PullVentoyCliLogIntoLogs(const std::wstring& exeDir, const std::wstring& logsDir,
                              const std::wstring& archiveDir) {
    const std::wstring ventoyCli = JoinPath(JoinPath(exeDir, L"Ventoy2Disk"), L"cli_log.txt");
    if (!FileExists(ventoyCli) || GetFileSizeBytes(ventoyCli) == 0) {
        return;
    }

    FILETIME ventoyWrite{};
    if (!GetFileLastWriteTime(ventoyCli, ventoyWrite)) {
        return;
    }

    // Skip stale Ventoy2Disk leftovers already captured in logs/ or a prior archive session.
    const FILETIME known = NewestKnownCliLogWriteTime(logsDir, archiveDir);
    const bool haveKnown = known.dwLowDateTime != 0 || known.dwHighDateTime != 0;
    if (haveKnown && !FileTimeNewer(ventoyWrite, known)) {
        return;
    }

    const std::wstring dest = JoinPath(logsDir, L"cli_log.txt");
    CopyFileW(ventoyCli.c_str(), dest.c_str(), FALSE);
}

bool LogsDirHasSessionFiles(const std::wstring& logsDir) {
    for (const wchar_t* name : kSessionLogFiles) {
        const std::wstring path = JoinPath(logsDir, name);
        if (FileExists(path) && GetFileSizeBytes(path) > 0) {
            return true;
        }
    }
    return false;
}

void RotateSessionLogsIntoArchive(const std::wstring& logsDir, const std::wstring& archiveDir) {
    if (!LogsDirHasSessionFiles(logsDir)) {
        return;
    }

    std::wstring folderName = FormatSessionArchiveFolderName();
    std::wstring sessionDir = JoinPath(archiveDir, folderName);
    if (GetFileAttributesW(sessionDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
        folderName += L"_" + std::to_wstring(GetTickCount64() % 100000);
        sessionDir = JoinPath(archiveDir, folderName);
    }
    CreateDirectoryW(sessionDir.c_str(), nullptr);

    for (const wchar_t* name : kSessionLogFiles) {
        const std::wstring src = JoinPath(logsDir, name);
        if (!FileExists(src)) {
            continue;
        }
        const std::wstring dest = JoinPath(sessionDir, name);
        if (!MoveFileW(src.c_str(), dest.c_str())) {
            // Keep going; truncate/delete so this session still starts clean.
            DeleteFileW(src.c_str());
        }
    }
}

}  // namespace

void PrepareInstallerLogs() {
    const std::wstring exeDir = GetExeDirectory();
    const std::wstring logsDir = GetLogsDirectory();
    const std::wstring archiveDir = JoinPath(logsDir, L"archive");
    CreateDirectoryW(logsDir.c_str(), nullptr);
    CreateDirectoryW(archiveDir.c_str(), nullptr);

    for (const wchar_t* name : kSessionLogFiles) {
        // cli_* live under Ventoy2Disk historically; others may sit beside the exe.
        if (wcsncmp(name, L"cli_", 4) == 0) {
            continue;
        }
        MigrateLegacyLogFile(exeDir, logsDir, archiveDir, name);
    }
    PullVentoyCliLogIntoLogs(exeDir, logsDir, archiveDir);
    RotateSessionLogsIntoArchive(logsDir, archiveDir);
    PruneLogSessionArchives(archiveDir);
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

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (len <= 1) {
        return {};
    }
    std::wstring out(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), len);
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

std::wstring FormatWindowsError(const DWORD error) {
    if (error == 0) {
        return L"Unknown error";
    }

    wchar_t* message = nullptr;
    const DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                         FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr, error, 0, reinterpret_cast<LPWSTR>(&message), 0, nullptr);
    std::wstring out;
    if (len != 0 && message) {
        out.assign(message, len);
        while (!out.empty() && (out.back() == L'\r' || out.back() == L'\n')) {
            out.pop_back();
        }
    } else {
        out = L"Windows error " + std::to_wstring(error);
    }
    if (message) {
        LocalFree(message);
    }
    return out;
}

namespace {

bool ShellOpenUrl(const wchar_t* url) {
    const HINSTANCE result = ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

bool ShellOpenExecutableWithArg(const wchar_t* exe, const wchar_t* arg) {
    const HINSTANCE result = ShellExecuteW(nullptr, nullptr, exe, arg, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

bool IsBrowserExecutable(const wchar_t* path) {
    if (!path || !*path) {
        return false;
    }

    const wchar_t* name = PathFindFileNameW(path);
    return _wcsicmp(name, L"chrome.exe") == 0 || _wcsicmp(name, L"msedge.exe") == 0 ||
           _wcsicmp(name, L"firefox.exe") == 0 || _wcsicmp(name, L"opera.exe") == 0 ||
           _wcsicmp(name, L"brave.exe") == 0 || _wcsicmp(name, L"iexplore.exe") == 0 ||
           _wcsicmp(name, L"vivaldi.exe") == 0;
}

std::wstring ExpandEnvironmentPath(const wchar_t* pathTemplate) {
    if (!pathTemplate || !*pathTemplate) {
        return {};
    }

    wchar_t expanded[MAX_PATH]{};
    const DWORD length = ExpandEnvironmentStringsW(pathTemplate, expanded, MAX_PATH);
    if (length == 0 || length > MAX_PATH) {
        return {};
    }
    return expanded;
}

bool TryKnownTorrentClients(const wchar_t* magnetUrl) {
    static const wchar_t* kTorrentClientPaths[] = {
        L"%LocalAppData%\\Programs\\qBittorrent\\qbittorrent.exe",
        L"%ProgramFiles%\\qBittorrent\\qbittorrent.exe",
        L"%ProgramFiles(x86)%\\qBittorrent\\qbittorrent.exe",
        L"%AppData%\\uTorrent\\uTorrent.exe",
        L"%ProgramFiles%\\BitTorrent\\BitTorrent.exe",
        L"%ProgramFiles(x86)%\\BitTorrent\\BitTorrent.exe",
        L"%ProgramFiles%\\Deluge\\deluge.exe",
        L"%ProgramFiles(x86)%\\Deluge\\deluge.exe",
        L"%ProgramFiles%\\Transmission\\transmission-qt.exe",
        L"%ProgramFiles(x86)%\\Transmission\\transmission-qt.exe",
    };

    for (const wchar_t* pathTemplate : kTorrentClientPaths) {
        const std::wstring path = ExpandEnvironmentPath(pathTemplate);
        if (path.empty() || !FileExists(path)) {
            continue;
        }
        if (ShellOpenExecutableWithArg(path.c_str(), magnetUrl)) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool OpenMagnetUrl(const wchar_t* url) {
    if (!url || _wcsnicmp(url, L"magnet:", 7) != 0) {
        return false;
    }

    wchar_t handlerExe[MAX_PATH]{};
    DWORD handlerExeLength = static_cast<DWORD>(std::size(handlerExe));
    const HRESULT assocResult =
        AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, L"magnet", url, handlerExe, &handlerExeLength);
    if (SUCCEEDED(assocResult) && handlerExe[0] != L'\0' && !IsBrowserExecutable(handlerExe)) {
        if (ShellOpenUrl(url)) {
            return true;
        }
        if (ShellOpenExecutableWithArg(handlerExe, url)) {
            return true;
        }
    }

    if (TryKnownTorrentClients(url)) {
        return true;
    }

    if (FAILED(assocResult) || handlerExe[0] == L'\0') {
        return ShellOpenUrl(url);
    }

    return false;
}

bool CopyTextToClipboard(HWND owner, const std::wstring& text) {
    if (text.empty()) {
        return false;
    }
    if (!OpenClipboard(owner)) {
        return false;
    }
    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) {
        CloseClipboard();
        return false;
    }
    void* locked = GlobalLock(memory);
    if (!locked) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    memcpy(locked, text.c_str(), bytes);
    GlobalUnlock(memory);
    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    CloseClipboard();
    return true;
}

}  // namespace medicat
