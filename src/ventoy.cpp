#include "ventoy.h"

#include "cancel.h"
#include "drives.h"
#include "download.h"
#include "extract.h"
#include "offline.h"
#include "resource.h"
#include "sim_fail.h"
#include "util.h"

#include <windows.h>

#include <atomic>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include <cstring>

namespace medicat {

namespace {

constexpr UINT32 kVentoyPart1StartSector = 2048;
constexpr UINT32 kVentoyEfiPartSectors = (32u * 1024u * 1024u) / 512u;  // 32 MiB EFI partition

#pragma pack(push, 1)
struct VentoyPartTable {
    BYTE Active;
    BYTE StartHead;
    UINT16 StartSector : 6;
    UINT16 StartCylinder : 10;
    BYTE FsFlag;
    BYTE EndHead;
    UINT16 EndSector : 6;
    UINT16 EndCylinder : 10;
    UINT32 StartSectorId;
    UINT32 SectorCount;
};

struct VentoyMbrHead {
    BYTE BootCode[446];
    VentoyPartTable PartTbl[4];
    BYTE Byte55;
    BYTE ByteAA;
};

struct VentoyGptHdr {
    CHAR Signature[8];
    BYTE Version[4];
    UINT32 Length;
    UINT32 Crc;
    BYTE Reserved1[4];
    UINT64 EfiStartLBA;
    UINT64 EfiBackupLBA;
    UINT64 PartAreaStartLBA;
    UINT64 PartAreaEndLBA;
    GUID DiskGuid;
    UINT64 PartTblStartLBA;
    UINT32 PartTblTotNum;
    UINT32 PartTblEntryLen;
    UINT32 PartTblCrc;
    BYTE Reserved2[420];
};

struct VentoyGptPartTbl {
    GUID PartType;
    GUID PartGuid;
    UINT64 StartLBA;
    UINT64 LastLBA;
    UINT64 Attr;
    WCHAR Name[36];
};

struct VentoyGptInfo {
    VentoyMbrHead MBR;
    VentoyGptHdr Head;
    VentoyGptPartTbl PartTbl[128];
};
#pragma pack(pop)

// Partition layout check aligned with Ventoy2Disk IsVentoyPhyDrive (VTOYEFI + 32 MiB part 2).
bool IsVentoyPhysicalDrive(const DWORD phyDrive) {
    std::wostringstream path;
    path << L"\\\\.\\PhysicalDrive" << phyDrive;
    const HANDLE disk =
        CreateFileW(path.str().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
                    nullptr);
    if (disk == INVALID_HANDLE_VALUE) {
        return false;
    }

    VentoyMbrHead mbr{};
    DWORD read = 0;
    if (!ReadFile(disk, &mbr, sizeof(mbr), &read, nullptr) || read != sizeof(mbr)) {
        CloseHandle(disk);
        return false;
    }

    if (mbr.Byte55 != 0x55 || mbr.ByteAA != 0xAA) {
        CloseHandle(disk);
        return false;
    }

    if (mbr.PartTbl[0].FsFlag == 0xEE) {
        VentoyGptInfo gpt{};
        SetFilePointer(disk, 0, nullptr, FILE_BEGIN);
        const BOOL gptRead = ReadFile(disk, &gpt, sizeof(gpt), &read, nullptr);
        CloseHandle(disk);
        if (!gptRead || read != sizeof(gpt)) {
            return false;
        }

        if (memcmp(gpt.Head.Signature, "EFI PART", 8) != 0) {
            return false;
        }

        static const WCHAR kVtoyEfiName[] = L"VTOYEFI";
        if (memcmp(gpt.PartTbl[1].Name, kVtoyEfiName, sizeof(kVtoyEfiName) - sizeof(WCHAR)) != 0) {
            return false;
        }

        if (gpt.PartTbl[0].StartLBA != kVentoyPart1StartSector) {
            return false;
        }

        const UINT32 part2SectorCount =
            static_cast<UINT32>(gpt.PartTbl[1].LastLBA + 1 - gpt.PartTbl[1].StartLBA);
        if (gpt.PartTbl[1].StartLBA != gpt.PartTbl[0].LastLBA + 1 ||
            part2SectorCount != kVentoyEfiPartSectors) {
            return false;
        }

        return true;
    }

    CloseHandle(disk);

    if (mbr.PartTbl[0].StartSectorId != kVentoyPart1StartSector) {
        return false;
    }

    const UINT32 part2Start = mbr.PartTbl[0].StartSectorId + mbr.PartTbl[0].SectorCount;
    if (mbr.PartTbl[1].StartSectorId != part2Start || mbr.PartTbl[1].SectorCount != kVentoyEfiPartSectors) {
        return false;
    }

    return true;
}

}  // namespace

class VentoyFileLog {
public:
    bool OpenNew(const std::wstring& path) {
        Close();
        file_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) {
            return false;
        }
        static constexpr char kHeader[] = "===== MediCat Ventoy log =====\r\n\r\n";
        WriteRaw(kHeader, sizeof(kHeader) - 1);
        return true;
    }

    bool OpenAppend(const std::wstring& path) {
        Close();
        file_ = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) {
            return false;
        }
        return true;
    }

    bool IsOpen() const { return file_ != INVALID_HANDLE_VALUE; }

    void WriteSection(const std::wstring& title) {
        if (!IsOpen()) {
            return;
        }
        std::string section = "\r\n----- ";
        section += WideToUtf8(title);
        section += " -----\r\n";
        WriteRaw(section.data(), section.size());
    }

    void WriteLine(const std::wstring& line) {
        if (!IsOpen()) {
            return;
        }
        std::string text = WideToUtf8(line);
        text += "\r\n";
        WriteRaw(text.data(), text.size());
    }

    void WriteStdout(const char* data, const size_t len) { WriteProcessOutput(data, len, false); }

    void WriteStderr(const char* data, const size_t len) { WriteProcessOutput(data, len, true); }

    bool AppendFile(const std::wstring& path) {
        if (!IsOpen() || !FileExists(path)) {
            return false;
        }
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return false;
        }
        char buf[8192];
        while (in.read(buf, sizeof(buf)) || in.gcount() > 0) {
            const std::streamsize count = in.gcount();
            if (count > 0) {
                WriteRaw(buf, static_cast<size_t>(count));
            }
        }
        return true;
    }

    void Close() {
        if (file_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_);
            file_ = INVALID_HANDLE_VALUE;
        }
        stderrMarkerWritten_ = false;
    }

private:
    void WriteProcessOutput(const char* data, const size_t len, const bool isStderr) {
        if (!data || len == 0 || !IsOpen()) {
            return;
        }
        std::lock_guard lock(mutex_);
        if (isStderr && !stderrMarkerWritten_) {
            static constexpr char kMarker[] = "\r\n----- stderr -----\r\n";
            WriteRawLocked(kMarker, sizeof(kMarker) - 1);
            stderrMarkerWritten_ = true;
        }
        WriteRawLocked(data, len);
    }

    void WriteRaw(const char* data, const size_t len) {
        std::lock_guard lock(mutex_);
        WriteRawLocked(data, len);
    }

    void WriteRawLocked(const char* data, const size_t len) {
        if (file_ == INVALID_HANDLE_VALUE || !data || len == 0) {
            return;
        }
        DWORD written = 0;
        WriteFile(file_, data, static_cast<DWORD>(len), &written, nullptr);
    }

    HANDLE file_ = INVALID_HANDLE_VALUE;
    std::mutex mutex_;
    bool stderrMarkerWritten_ = false;
};

namespace {

void LogLine(const VentoyEnsureOptions& options, VentoyFileLog* fileLog, const std::wstring& message) {
    if (options.onLog) {
        options.onLog(message);
    }
    if (fileLog && fileLog->IsOpen()) {
        fileLog->WriteLine(message);
    }
}

void SetStatus(const VentoyEnsureOptions& options, const std::wstring& message) {
    if (options.onStatus) {
        options.onStatus(message);
    }
}

int RunHiddenProcess(const std::wstring& commandLine, const std::wstring& workingDir = L"") {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> cmd(commandLine.begin(), commandLine.end());
    cmd.push_back(L'\0');

    PROCESS_INFORMATION pi{};
    const wchar_t* work = workingDir.empty() ? nullptr : workingDir.c_str();
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, work,
                        &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(code);
}

int RunProcessWithLog(const std::wstring& commandLine, const std::wstring& workingDir, VentoyFileLog& log) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        return -1;
    }
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    HANDLE stderrRead = nullptr;
    HANDLE stderrWrite = nullptr;
    if (!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        return -1;
    }
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmd(commandLine.begin(), commandLine.end());
    cmd.push_back(L'\0');
    const wchar_t* work = workingDir.empty() ? nullptr : workingDir.c_str();

    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, work, &si,
                        &pi)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);
        return -1;
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    std::atomic<bool> running{true};
    std::thread stderrThread([&] {
        char buf[4096];
        DWORD n = 0;
        while (running.load()) {
            if (!ReadFile(stderrRead, buf, sizeof(buf), &n, nullptr) || n == 0) {
                break;
            }
            if (log.IsOpen()) {
                log.WriteStderr(buf, n);
            }
        }
    });

    while (true) {
        DWORD avail = 0;
        if (!PeekNamedPipe(stdoutRead, nullptr, 0, nullptr, &avail, nullptr)) {
            break;
        }
        if (avail == 0) {
            DWORD wait = WaitForSingleObject(pi.hProcess, 50);
            if (wait == WAIT_OBJECT_0) {
                break;
            }
            continue;
        }

        std::string chunk(avail, '\0');
        DWORD read = 0;
        if (!ReadFile(stdoutRead, chunk.data(), avail, &read, nullptr) || read == 0) {
            break;
        }
        chunk.resize(read);
        if (log.IsOpen()) {
            log.WriteStdout(chunk.data(), chunk.size());
        }
    }

    while (true) {
        char buf[4096];
        DWORD n = 0;
        if (!ReadFile(stdoutRead, buf, sizeof(buf), &n, nullptr) || n == 0) {
            break;
        }
        if (log.IsOpen()) {
            log.WriteStdout(buf, n);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    running.store(false);
    if (stderrThread.joinable()) {
        stderrThread.join();
    }

    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    return static_cast<int>(code);
}

bool PathExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

constexpr int kVentoyIoRetries = 3;
constexpr DWORD kVentoyIoRetryDelayMs = 500;
constexpr int kVentoyDownloadRetries = 3;
constexpr DWORD kVentoyDownloadRetryDelayMs = 1500;
constexpr int kVentoyExtractRetries = 2;
constexpr DWORD kVentoyExtractRetryDelayMs = 1000;
constexpr int kVentoyLayoutRetries = 4;
constexpr DWORD kVentoyLayoutRetryDelayMs = 400;

bool SleepUnlessCancelled(const DWORD delayMs) {
    const DWORD step = 100;
    for (DWORD waited = 0; waited < delayMs; waited += step) {
        if (IsCancelRequested()) {
            return false;
        }
        Sleep(step);
    }
    return !IsCancelRequested();
}

bool IsRetriableWin32Error(const DWORD error) {
    switch (error) {
        case ERROR_SHARING_VIOLATION:
        case ERROR_ACCESS_DENIED:
        case ERROR_LOCK_VIOLATION:
        case ERROR_DIR_NOT_EMPTY:
        case ERROR_USER_MAPPED_FILE:
            return true;
        default:
            return false;
    }
}

bool RemoveDirectoryTree(const std::wstring& path) {
    if (path.empty() || path.size() < 3) {
        return false;
    }
    std::wstring pattern = JoinPath(path, L"*");
    WIN32_FIND_DATAW fd{};
    const HANDLE find = FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return RemoveDirectoryW(path.c_str()) != FALSE;
    }

    bool ok = true;
    do {
        const std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        const std::wstring full = JoinPath(path, name);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!RemoveDirectoryTree(full)) {
                ok = false;
            }
        } else {
            SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFileW(full.c_str())) {
                ok = false;
            }
        }
    } while (FindNextFileW(find, &fd));
    FindClose(find);
    if (!RemoveDirectoryW(path.c_str())) {
        ok = false;
    }
    return ok;
}

void CleanupPartialVentoyExtract(const std::wstring& root, const std::wstring& targetVersion) {
    const std::wstring preferred = JoinPath(root, L"ventoy-" + targetVersion);
    if (PathExists(preferred)) {
        RemoveDirectoryTree(preferred);
    }
}

std::wstring ListDirectoryEntries(const std::wstring& path, const size_t maxEntries) {
    const std::wstring pattern = JoinPath(path, L"*");
    WIN32_FIND_DATAW fd{};
    const HANDLE find = FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return L"(empty or inaccessible)";
    }

    std::wstring listing;
    size_t count = 0;
    do {
        const std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        if (!listing.empty()) {
            listing += L", ";
        }
        listing += name;
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            listing += L"/";
        }
        ++count;
        if (count >= maxEntries) {
            listing += L", ...";
            break;
        }
    } while (FindNextFileW(find, &fd));
    FindClose(find);
    return listing.empty() ? L"(empty)" : listing;
}

std::wstring FindExtractedVentoyDir(const std::wstring& root, const std::wstring& preferred) {
    if (FileExists(JoinPath(preferred, L"Ventoy2Disk.exe"))) {
        return preferred;
    }

    const std::wstring pattern = JoinPath(root, L"*");
    WIN32_FIND_DATAW fd{};
    const HANDLE find = FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return {};
    }

    std::wstring match;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            continue;
        }
        const std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        const std::wstring candidate = JoinPath(root, name);
        if (!FileExists(JoinPath(candidate, L"Ventoy2Disk.exe"))) {
            continue;
        }
        if (name.rfind(L"ventoy-", 0) == 0 || name == L"Ventoy2Disk") {
            return candidate;
        }
        if (match.empty()) {
            match = candidate;
        }
    } while (FindNextFileW(find, &fd));
    FindClose(find);
    return match;
}

bool RelocateExistingDirectoryOnce(const std::wstring& path, std::wstring& errorDetail, DWORD& failureCode) {
    failureCode = 0;
    if (!PathExists(path)) {
        return true;
    }

    if (RemoveDirectoryTree(path)) {
        return true;
    }

    const std::wstring backup = path + L".old";
    if (PathExists(backup)) {
        RemoveDirectoryTree(backup);
    }

    if (MoveFileExW(path.c_str(), backup.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return true;
    }

    failureCode = GetLastError();
    errorDetail = L"Could not remove existing folder: " + ShortDisplayPath(path, 80) + L"\n";
    errorDetail += L"Move aside failed: " + FormatWindowsError(failureCode) + L" (" +
                   std::to_wstring(failureCode) + L")\n";
    errorDetail += L"Contents: " + ListDirectoryEntries(path, 8);
    errorDetail += L"\nClose other programs using Ventoy2Disk.exe, then retry.";
    return false;
}

bool FinalizeVentoyExtract(const VentoyEnsureOptions& options, VentoyFileLog* fileLog, const std::wstring& extractedDir,
                           const std::wstring& ventoyDir, std::wstring& errorDetail) {
    DWORD failureCode = 0;
    for (int attempt = 1; attempt <= kVentoyIoRetries; ++attempt) {
        if (attempt > 1) {
            LogLine(options, fileLog, L"Retrying Ventoy folder install (attempt " + std::to_wstring(attempt) + L"/" +
                                             std::to_wstring(kVentoyIoRetries) + L")");
            if (!SleepUnlessCancelled(kVentoyIoRetryDelayMs)) {
                errorDetail = L"Cancelled";
                return false;
            }
        }

        std::wstring relocateError;
        if (!RelocateExistingDirectoryOnce(ventoyDir, relocateError, failureCode)) {
            errorDetail = relocateError;
            if (attempt < kVentoyIoRetries && IsRetriableWin32Error(failureCode)) {
                continue;
            }
            return false;
        }

        if (MoveFileW(extractedDir.c_str(), ventoyDir.c_str())) {
            return true;
        }

        failureCode = GetLastError();
        errorDetail = L"From: " + ShortDisplayPath(extractedDir, 80) + L"\n";
        errorDetail += L"To: " + ShortDisplayPath(ventoyDir, 80) + L"\n";
        errorDetail += FormatWindowsError(failureCode) + L" (" + std::to_wstring(failureCode) + L")";
        if (attempt < kVentoyIoRetries && IsRetriableWin32Error(failureCode)) {
            continue;
        }
        return false;
    }
    return false;
}

bool DownloadAndInstallVentoy(const VentoyEnsureOptions& options, VentoyFileLog* fileLog,
                                const std::wstring& targetVersion, const std::wstring& ventoyDir,
                                VentoyResult& result) {
    const std::wstring cachedZip = GetOfflineVentoyZipPath(targetVersion);
    const std::wstring workingZip = JoinPath(options.root, L"ventoy.zip");
    std::wstring zipPath;
    bool deleteWorkingZip = false;
    const std::wstring zipUrl = L"https://github.com/ventoy/Ventoy/releases/download/v" + targetVersion + L"/ventoy-" +
                                targetVersion + L"-windows.zip";

    if (OfflineVentoyZipExists(targetVersion)) {
        LogLine(options, fileLog, L"Using offline Ventoy cache for v" + targetVersion);
        zipPath = cachedZip;
    } else {
        zipPath = workingZip;
        deleteWorkingZip = true;

        std::wstring downloadError;
        bool downloaded = false;
        for (int attempt = 1; attempt <= kVentoyDownloadRetries; ++attempt) {
            if (IsCancelRequested()) {
                return false;
            }
            if (attempt > 1) {
                LogLine(options, fileLog, L"Retrying Ventoy download (attempt " + std::to_wstring(attempt) + L"/" +
                                                 std::to_wstring(kVentoyDownloadRetries) + L")");
                DeleteFileW(zipPath.c_str());
                if (!SleepUnlessCancelled(kVentoyDownloadRetryDelayMs)) {
                    return false;
                }
            }

            SetStatus(options, L"downloading_ventoy:" + targetVersion);
            if (HttpDownloadFile(zipUrl, zipPath, downloadError)) {
                downloaded = true;
                break;
            }
        }

        if (!downloaded) {
            result.failureKind = VentoyResult::FailureKind::Download;
            result.error = downloadError;
            return false;
        }

        CacheVentoyZip(targetVersion, zipPath);
    }

    ExtractResult extract;
    bool extracted = false;
    for (int attempt = 1; attempt <= kVentoyExtractRetries; ++attempt) {
        if (IsCancelRequested()) {
            return false;
        }
        if (attempt > 1) {
            LogLine(options, fileLog, L"Retrying Ventoy extraction (attempt " + std::to_wstring(attempt) + L"/" +
                                             std::to_wstring(kVentoyExtractRetries) + L")");
            CleanupPartialVentoyExtract(options.root, targetVersion);
            if (!SleepUnlessCancelled(kVentoyExtractRetryDelayMs)) {
                return false;
            }
        }

        SetStatus(options, L"extracting_ventoy");
        if (attempt == 1) {
            LogLine(options, fileLog, L"Extracting Ventoy archive");
            if (fileLog) {
                fileLog->WriteSection(L"Ventoy archive extract (7za)");
            }
        }

        const std::wstring extractTmpLog = JoinPath(GetMedicatTempDir(), L"ventoy_7za_extract.log");
        DeleteFileW(extractTmpLog.c_str());

        extract = Extract7zArchive(
            options.sevenZipExe, zipPath, options.root, 0, 0,
            [&](const ExtractProgress& progress) {
                if (progress.percent >= 0) {
                    SetStatus(options, L"extracting_ventoy:" + std::to_wstring(progress.percent));
                }
            },
            extractTmpLog, false);

        if (fileLog) {
            fileLog->AppendFile(extractTmpLog);
            DeleteFileW(extractTmpLog.c_str());
        }

        if (extract.success) {
            extracted = true;
            break;
        }
    }

    if (deleteWorkingZip) {
        DeleteFileW(zipPath.c_str());
    }

    if (!extracted) {
        result.failureKind = VentoyResult::FailureKind::Extract;
        result.exitCode = extract.exitCode;
        result.error = extract.error;
        if (!extract.detail.empty()) {
            result.error += L"\n" + extract.detail;
        }
        return false;
    }

    const std::wstring preferredDir = JoinPath(options.root, L"ventoy-" + targetVersion);
    std::wstring extractedDir;
    for (int attempt = 1; attempt <= kVentoyLayoutRetries; ++attempt) {
        if (IsCancelRequested()) {
            return false;
        }
        if (attempt > 1) {
            LogLine(options, fileLog, L"Retrying Ventoy layout check (attempt " + std::to_wstring(attempt) + L"/" +
                                             std::to_wstring(kVentoyLayoutRetries) + L")");
            if (!SleepUnlessCancelled(kVentoyLayoutRetryDelayMs)) {
                return false;
            }
        }

        extractedDir = FindExtractedVentoyDir(options.root, preferredDir);
        if (!extractedDir.empty()) {
            break;
        }
    }

    if (extractedDir.empty()) {
        result.failureKind = VentoyResult::FailureKind::Layout;
        result.error = L"Expected folder: ventoy-" + targetVersion + L"\n";
        result.error += L"Extracted to: " + ShortDisplayPath(options.root, 80) + L"\n";
        result.error += L"Contents: " + ListDirectoryEntries(options.root, 12);
        LogLine(options, fileLog, L"Ventoy layout check failed — root listing: " +
                                         ListDirectoryEntries(options.root, 12));
        return false;
    }

    if (extractedDir != preferredDir) {
        LogLine(options, fileLog, L"Using extracted Ventoy folder: " + extractedDir);
    }

    std::wstring installError;
    if (!FinalizeVentoyExtract(options, fileLog, extractedDir, ventoyDir, installError)) {
        if (installError == L"Cancelled") {
            return false;
        }
        result.failureKind = VentoyResult::FailureKind::Rename;
        result.error = installError;
        LogLine(options, fileLog, L"Ventoy folder install failed: " + installError);
        return false;
    }

    LogLine(options, fileLog, L"Ventoy v" + targetVersion + L" ready");
    return true;
}

std::wstring ReadTextFile(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary);
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
    std::wstring wide(static_cast<size_t>(len > 0 ? len - 1 : 0), L'\0');
    if (len > 0) {
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, wide.data(), len);
    }
    return wide;
}

std::wstring ReadTextFileFull(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return L"";
    }
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (content.empty()) {
        return L"";
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring wide(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), wide.data(), len);
    return wide;
}

void ClearVentoyCliArtifacts(const std::wstring& ventoyDir) {
    static const wchar_t* kFiles[] = {L"cli_log.txt", L"cli_done.txt", L"cli_percent.txt", nullptr};
    for (const wchar_t* const* name = kFiles; *name != nullptr; ++name) {
        DeleteFileW(JoinPath(ventoyDir, *name).c_str());
    }
}

bool VentoyCliLineLooksImportant(const std::wstring& line) {
    if (line.empty()) {
        return false;
    }
    std::wstring upper = line;
    for (wchar_t& ch : upper) {
        if (ch >= L'a' && ch <= L'z') {
            ch = static_cast<wchar_t>(ch - L'a' + L'A');
        }
    }
    static const wchar_t* kNeedles[] = {L"FAILED", L"ERROR", L"LASTERR", L"CANNOT", L"UNABLE", L"REFUSE",
                                        L"INVALID", L"NOT FOUND", L"NO SUCH", nullptr};
    for (const wchar_t* const* needle = kNeedles; *needle != nullptr; ++needle) {
        if (upper.find(*needle) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

std::wstring ExtractVentoyCliLogExcerpt(const std::wstring& cliLogPath) {
    const std::wstring content = ReadTextFileFull(cliLogPath);
    if (content.empty()) {
        return L"";
    }

    std::vector<std::wstring> highlights;
    size_t start = 0;
    while (start <= content.size()) {
        const size_t end = content.find(L'\n', start);
        const size_t lineEnd = end == std::wstring::npos ? content.size() : end;
        std::wstring line = content.substr(start, lineEnd - start);
        while (!line.empty() && (line.back() == L'\r' || line.back() == L' ' || line.back() == L'\t')) {
            line.pop_back();
        }
        if (VentoyCliLineLooksImportant(line)) {
            highlights.push_back(line);
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }

    if (highlights.empty()) {
        std::vector<std::wstring> tail;
        start = 0;
        while (start <= content.size()) {
            const size_t end = content.find(L'\n', start);
            const size_t lineEnd = end == std::wstring::npos ? content.size() : end;
            std::wstring line = content.substr(start, lineEnd - start);
            while (!line.empty() && (line.back() == L'\r' || line.back() == L' ' || line.back() == L'\t')) {
                line.pop_back();
            }
            if (!line.empty()) {
                tail.push_back(line);
            }
            if (end == std::wstring::npos) {
                break;
            }
            start = end + 1;
        }
        const size_t keep = tail.size() < 12 ? tail.size() : 12;
        highlights.assign(tail.end() - static_cast<std::ptrdiff_t>(keep), tail.end());
    } else if (highlights.size() > 12) {
        highlights.erase(highlights.begin(), highlights.end() - 12);
    }

    std::wstring excerpt;
    for (const std::wstring& line : highlights) {
        if (!excerpt.empty()) {
            excerpt += L"\n";
        }
        excerpt += line;
    }
    return excerpt;
}

void AppendVentoyCliArtifacts(const std::wstring& ventoyDir, VentoyFileLog& fileLog) {
    static const wchar_t* kFiles[] = {L"cli_log.txt", L"cli_done.txt", L"cli_percent.txt", nullptr};
    for (const wchar_t* const* name = kFiles; *name != nullptr; ++name) {
        const std::wstring path = JoinPath(ventoyDir, *name);
        if (!FileExists(path) || GetFileSizeBytes(path) == 0) {
            continue;
        }
        fileLog.WriteSection(std::wstring(*name) + L" (from Ventoy2Disk/)");
        fileLog.AppendFile(path);
    }
}

std::wstring NormalizeVersion(std::wstring version) {
    while (!version.empty() && (version.front() == L'v' || version.front() == L'V')) {
        version.erase(version.begin());
    }
    return version;
}

std::wstring ParseLatestTagFromGitHubJson(const std::wstring& json) {
    const std::wstring needle = L"\"tag_name\":\"";
    const size_t pos = json.find(needle);
    if (pos == std::wstring::npos) {
        return L"";
    }

    const size_t start = pos + needle.size();
    const size_t end = json.find(L'"', start);
    if (end == std::wstring::npos || end <= start) {
        return L"";
    }
    return json.substr(start, end - start);
}

std::vector<std::wstring> ParseReleaseTagsFromGitHubJson(const std::wstring& json) {
    std::vector<std::wstring> tags;
    const std::wstring needle = L"\"tag_name\":\"";
    size_t pos = 0;
    while ((pos = json.find(needle, pos)) != std::wstring::npos) {
        const size_t start = pos + needle.size();
        const size_t end = json.find(L'"', start);
        if (end != std::wstring::npos && end > start) {
            const std::wstring version = NormalizeVersion(json.substr(start, end - start));
            if (!version.empty()) {
                tags.push_back(version);
            }
        }
        pos = start;
    }
    return tags;
}

bool ParseVentoyVersionText(const std::wstring& content, std::vector<std::wstring>& versions) {
    versions.clear();
    if (content.empty()) {
        return false;
    }

    size_t start = 0;
    while (start <= content.size()) {
        const size_t end = content.find(L'\n', start);
        const size_t lineEnd = end == std::wstring::npos ? content.size() : end;
        std::wstring line = content.substr(start, lineEnd - start);
        while (!line.empty() && (line.back() == L'\r' || line.back() == L' ' || line.back() == L'\t')) {
            line.pop_back();
        }
        const std::wstring version = NormalizeVersion(line);
        if (!version.empty()) {
            versions.push_back(version);
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }

    return !versions.empty();
}

}  // namespace

VentoyResult FetchLatestVentoyVersion(std::wstring& version) {
    VentoyResult result;
    std::wstring body;
    std::wstring error;
    if (!HttpGet(L"https://api.github.com/repos/ventoy/Ventoy/releases/latest", body, error)) {
        if (ResolveOfflineVentoyVersion(L"", version)) {
            result.success = true;
            result.version = version;
            return result;
        }
        result.error = L"Could not fetch Ventoy version: " + error;
        return result;
    }

    version = NormalizeVersion(ParseLatestTagFromGitHubJson(body));
    if (version.empty()) {
        if (ResolveOfflineVentoyVersion(L"", version)) {
            result.success = true;
            result.version = version;
            return result;
        }
        result.error = L"Could not parse latest Ventoy version from GitHub";
        return result;
    }

    result.success = true;
    result.version = version;
    return result;
}

VentoyResult FetchVentoyVersions(std::vector<std::wstring>& versions) {
    VentoyResult result;
    versions.clear();

    for (int page = 1; page <= 20; ++page) {
        std::wstring body;
        std::wstring error;
        const std::wstring url = L"https://api.github.com/repos/ventoy/Ventoy/releases?per_page=100&page=" +
                                 std::to_wstring(page);
        if (!HttpGet(url, body, error)) {
            if (versions.empty()) {
                result.error = L"Could not fetch Ventoy versions: " + error;
            }
            break;
        }

        const std::vector<std::wstring> pageVersions = ParseReleaseTagsFromGitHubJson(body);
        if (pageVersions.empty()) {
            break;
        }

        versions.insert(versions.end(), pageVersions.begin(), pageVersions.end());
        if (pageVersions.size() < 100) {
            break;
        }
    }

    if (versions.empty()) {
        if (result.error.empty()) {
            result.error = L"Could not parse Ventoy versions from GitHub";
        }
        return result;
    }

    result.success = true;
    return result;
}

bool LoadBundledVentoyVersionList(const HINSTANCE instance, std::vector<std::wstring>& versions) {
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(IDR_VENTOY_VERSIONS), RT_RCDATA);
    if (!resource) {
        return false;
    }

    const HGLOBAL loaded = LoadResource(instance, resource);
    if (!loaded) {
        return false;
    }

    const DWORD size = SizeofResource(instance, resource);
    const void* data = LockResource(loaded);
    if (!data || size == 0) {
        return false;
    }

    const int len = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(data), static_cast<int>(size),
                                        nullptr, 0);
    if (len <= 0) {
        return false;
    }

    std::wstring content(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(data), static_cast<int>(size), content.data(), len);
    return ParseVentoyVersionText(content, versions);
}

VentoyResult EnsureVentoyReady(const VentoyEnsureOptions& options) {
    VentoyResult result;

    VentoyFileLog fileLog;
    VentoyFileLog* fileLogPtr = nullptr;
    if (!options.logPath.empty() && fileLog.OpenNew(options.logPath)) {
        fileLogPtr = &fileLog;
    }

    std::wstring targetVersion = NormalizeVersion(options.pinVersion);
    if (targetVersion.empty()) {
        SetStatus(options, L"checking_ventoy");
        const VentoyResult latest = FetchLatestVentoyVersion(targetVersion);
        if (!latest.success) {
            return latest;
        }
        LogLine(options, fileLogPtr, L"Latest Ventoy version: v" + targetVersion);
    } else {
        LogLine(options, fileLogPtr, L"Using pinned Ventoy version: v" + targetVersion);
    }

    const SimulatedFailure ventoyPrepareKinds[] = {SimulatedFailure::VentoyDownload, SimulatedFailure::VentoyExtract,
                                                   SimulatedFailure::VentoyLayout, SimulatedFailure::VentoyRename,
                                                   SimulatedFailure::VentoyPrepare};
    for (const SimulatedFailure kind : ventoyPrepareKinds) {
        if (ConsumeSimulatedFailure(kind)) {
            LogLine(options, fileLogPtr, std::wstring(L"[Debug] Simulating: ") + SimulatedFailureLabel(kind));
            return MakeSimulatedVentoyFailure(kind);
        }
    }

    const std::wstring ventoyDir = JoinPath(options.root, L"Ventoy2Disk");
    const std::wstring ventoyExe = JoinPath(ventoyDir, L"Ventoy2Disk.exe");
    const std::wstring versionFile = JoinPath(ventoyDir, L"ventoy\\version");
    const std::wstring localVersion = NormalizeVersion(ReadTextFile(versionFile));

    const bool needsDownload =
        !FileExists(ventoyExe) || localVersion.empty() || localVersion != targetVersion;

    if (needsDownload) {
        if (!localVersion.empty() && localVersion != targetVersion) {
            LogLine(options, fileLogPtr, L"Updating Ventoy from v" + localVersion + L" to v" + targetVersion);
        } else if (!OfflineVentoyZipExists(targetVersion)) {
            LogLine(options, fileLogPtr, L"Downloading Ventoy v" + targetVersion);
        }

        if (!DownloadAndInstallVentoy(options, fileLogPtr, targetVersion, ventoyDir, result)) {
            if (IsCancelRequested()) {
                return result;
            }
            if (!result.error.empty() || result.failureKind != VentoyResult::FailureKind::None) {
                return result;
            }
        }
    } else {
        LogLine(options, fileLogPtr, L"Local Ventoy v" + localVersion + L" is up to date");
    }

    if (!FileExists(ventoyExe)) {
        if (!needsDownload) {
            LogLine(options, fileLogPtr,
                    L"Ventoy2Disk.exe missing despite version file — clearing stale install and re-downloading");
            RemoveDirectoryTree(ventoyDir);
            if (!DownloadAndInstallVentoy(options, fileLogPtr, targetVersion, ventoyDir, result)) {
                if (IsCancelRequested()) {
                    return result;
                }
                if (!result.error.empty() || result.failureKind != VentoyResult::FailureKind::None) {
                    return result;
                }
            }
        }
    }

    if (!FileExists(ventoyExe)) {
        result.failureKind = VentoyResult::FailureKind::Prepare;
        result.error = L"Expected: " + ShortDisplayPath(ventoyExe, 80);
        if (!localVersion.empty()) {
            result.error += L"\nVersion file reports v" + localVersion + L" but Ventoy2Disk.exe is missing.";
        } else {
            result.error += L"\nVentoy2Disk.exe is missing after prepare.";
        }
        result.error += L"\nFolder contents: " + ListDirectoryEntries(ventoyDir, 12);
        LogLine(options, fileLogPtr, result.error);
        return result;
    }

    result.success = true;
    result.ventoyExe = ventoyExe;
    result.version = targetVersion;
    return result;
}

VentoyDetectionResult DetectVentoyOnDrive(const std::wstring& driveLetter, const std::wstring& logPath) {
    VentoyDetectionResult result;
    VentoyFileLog fileLog;
    const bool logging = !logPath.empty() && fileLog.OpenAppend(logPath);
    if (logging) {
        fileLog.WriteSection(L"Ventoy detection");
    }

    auto log = [&](const std::wstring& line) {
        result.logLines.push_back(line);
        if (logging) {
            fileLog.WriteLine(line);
        }
    };

    if (driveLetter.size() < 2) {
        log(L"Ventoy detection: invalid drive letter");
        return result;
    }

    log(L"Ventoy detection on " + driveLetter + L":");

    const DriveIdentity identity = GetDriveIdentity(driveLetter);
    bool layoutMatched = false;
    if (!identity.valid) {
        log(L"  physical disk: could not resolve drive identity");
    } else {
        std::wstring diskList;
        for (const DWORD diskNumber : identity.diskNumbers) {
            if (!diskList.empty()) {
                diskList += L", ";
            }
            diskList += L"PhysicalDrive" + std::to_wstring(diskNumber);
        }
        log(L"  physical disk(s): " + diskList);

        for (const DWORD diskNumber : identity.diskNumbers) {
            const bool layoutMatch = IsVentoyPhysicalDrive(diskNumber);
            log(L"  VTOYEFI layout on PhysicalDrive" + std::to_wstring(diskNumber) + L": " +
                (layoutMatch ? L"matched (Ventoy2Disk partition layout)" : L"not matched"));
            if (layoutMatch) {
                layoutMatched = true;
            }
        }
    }

    result.installed = layoutMatched;

    if (result.installed) {
        log(L"  result: Ventoy found (VTOYEFI partition layout)");
    } else {
        log(L"  result: Ventoy not found");
    }

    return result;
}

bool TestVentoyInstalled(const std::wstring& driveLetter) {
    return DetectVentoyOnDrive(driveLetter).installed;
}

VentoyResult RunVentoyInstall(const std::wstring& ventoyExe, const std::wstring& driveLetter,
                              const bool upgrade, const VentoyInstallOptions& options) {
    VentoyResult result;
    if (!FileExists(ventoyExe)) {
        result.error = L"Ventoy2Disk.exe not found";
        return result;
    }

    std::wstring drive = driveLetter;
    if (drive.size() >= 2 && drive[1] == L':') {
        drive = drive.substr(0, 2);
    }

    const std::wstring ventoyWork = GetExeDirectory();
    const std::wstring ventoyDir = JoinPath(ventoyWork, L"Ventoy2Disk");
    std::wstring args =
        upgrade ? (L"VTOYCLI /U /Drive:" + drive + L" /Y")
                : (L"VTOYCLI /I /Drive:" + drive + L" /NOUSBCheck /Y");
    if (options.useGpt) {
        args += L" /GPT";
    }
    if (!options.enableSecureBoot) {
        args += L" /NOSB";
    }

    std::wstring cmd = L"\"" + ventoyExe + L"\" " + args;

    VentoyFileLog fileLog;
    const bool logging = !options.logPath.empty() && fileLog.OpenAppend(options.logPath);
    if (logging) {
        fileLog.WriteSection(upgrade ? L"Ventoy upgrade (Ventoy2Disk.exe)" : L"Ventoy install (Ventoy2Disk.exe)");
        fileLog.WriteLine(L"Command: " + cmd);
        fileLog.WriteLine(L"Working directory: " + ventoyDir);
        fileLog.WriteLine(
            L"Note: Ventoy2Disk VTOYCLI writes diagnostics to cli_log.txt in the working directory (not stdout).");
    }

    ClearVentoyCliArtifacts(ventoyDir);

    if (logging) {
        result.exitCode = RunProcessWithLog(cmd, ventoyDir, fileLog);
        fileLog.WriteLine(L"Process exit code: " + std::to_wstring(result.exitCode));
        AppendVentoyCliArtifacts(ventoyDir, fileLog);
    } else {
        result.exitCode = RunHiddenProcess(cmd, ventoyDir);
    }

    const std::wstring cliLogPath = JoinPath(ventoyDir, L"cli_log.txt");
    result.cliLogExcerpt = ExtractVentoyCliLogExcerpt(cliLogPath);
    result.success = (result.exitCode == 0);
    result.ventoyExe = ventoyExe;
    if (!result.success) {
        std::wostringstream err;
        err << L"Ventoy failed with exit code " << result.exitCode;
        result.error = err.str();
        if (!result.cliLogExcerpt.empty()) {
            result.error += L"\n\n" + result.cliLogExcerpt;
        } else if (!FileExists(cliLogPath)) {
            result.error += L"\n\n(Ventoy cli_log.txt was not created — check Ventoy2Disk folder beside installer)";
        } else {
            result.error += L"\n\n(Ventoy cli_log.txt was empty — see ventoy.log)";
        }
    }
    return result;
}

bool FormatDriveNtfs(const std::wstring& driveLetter, const std::wstring& label) {
    if (driveLetter.empty()) {
        return false;
    }
    wchar_t letter = driveLetter[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    if (letter < L'A' || letter > L'Z') {
        return false;
    }
    const std::wstring drive = std::wstring(1, letter) + L":";
    const std::wstring cmd = L"format.com " + drive + L" /FS:NTFS /X /Q /V:" + label + L" /Y";
    return RunHiddenProcess(cmd) == 0;
}

}  // namespace medicat
