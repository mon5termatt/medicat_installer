#include "verify.h"

#include "cancel.h"
#include "download.h"
#include "offline.h"
#include "util.h"

#include <shlwapi.h>
#include <windows.h>
#include <wincrypt.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace medicat {

namespace {

constexpr size_t kMaxStoredFailures = 10000;
constexpr size_t kProgressReportInterval = 50;

constexpr wchar_t kMd5DownloadUrlHasher[] =
    L"https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/MedicatFiles.md5";
constexpr wchar_t kMd5DownloadUrlPrimary[] =
    L"https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/MedicatFiles.md5";

struct Md5Entry {
    std::wstring relativePath;
    std::string expectedHash;
};

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring out(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), len);
    return out;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0,
                                        nullptr, nullptr);
    if (len <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), len, nullptr,
                        nullptr);
    return out;
}

std::string TrimAscii(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) {
        value.erase(value.begin());
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
        value.pop_back();
    }
    return value;
}

bool IsHexLower(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

std::string ToLowerAscii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::wstring NormalizeRelativePath(std::wstring path) {
    while (!path.empty() && (path.front() == L'\\' || path.front() == L'/')) {
        path.erase(path.begin());
    }
    for (wchar_t& ch : path) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return path;
}

bool ShouldSkipKnownFile(const std::wstring& relativePath) {
    // autorun.ico is optional on MediCat USB installs (often absent or recreated by Windows).
    return NormalizeRelativePath(relativePath) == L"autorun.ico";
}

std::wstring ToLongPath(const std::wstring& path) {
    if (path.empty() || path.rfind(L"\\\\?\\", 0) == 0) {
        return path;
    }
    if (path.size() >= 2 && path[1] == L':') {
        return L"\\\\?\\" + path;
    }
    return path;
}

std::wstring ResolveVerifyPath(const std::wstring& driveRoot, const std::wstring& relativePath) {
    std::wstring root = driveRoot;
    if (root.size() == 2 && root[1] == L':') {
        root += L'\\';
    }

    std::wstring combined = root;
    if (!combined.empty() && combined.back() != L'\\' && combined.back() != L'/') {
        combined += L'\\';
    }
    combined += relativePath;
    return ToLongPath(combined);
}

bool PathMarkerExists(const std::wstring& path, const bool expectDirectory) {
    const DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    const bool isDirectory = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    return expectDirectory ? isDirectory : !isDirectory;
}

// Top-level MediCat.USB archive layout — weighted for a 0–100% presence score.
const MedicatPresenceMarker kMedicatPresenceMarkers[] = {
    {L"Start.exe", false, 35},
    {L"PortableApps", true, 15},
    {L"Programs", true, 15},
    {L"System", true, 10},
    {L"Antivirus", true, 5},
    {L"Boot_Repair", true, 5},
    {L"Backup_and_Recovery", true, 4},
    {L"Diagnostic_Tools", true, 4},
    {L"Live_Operating_Systems", true, 4},
    {L"OSimages", true, 4},
    {L"Partition_Tools", true, 4},
    {L"Password_Removal", true, 4},
    {L"Windows_Recovery", true, 4},
    {L"VHD", true, 3},
    {L"Backup", true, 3},
    {L"ventoy", true, 3},
    {L"LICENSE.txt", false, 3},
    {L"autorun.inf", false, 3},
    {L"CDUsb.y", false, 2},
};

bool ParseMd5Line(const std::string& line, Md5Entry& entry) {
    if (line.empty() || line[0] == ';') {
        return false;
    }

    const size_t space = line.find_first_of(" \t");
    if (space == std::string::npos) {
        return false;
    }

    std::string hash = TrimAscii(line.substr(0, space));
    if (hash.size() != 32) {
        return false;
    }
    for (const char ch : hash) {
        if (!IsHexLower(ch)) {
            return false;
        }
    }

    std::string path = TrimAscii(line.substr(space + 1));
    if (!path.empty() && path.front() == '*') {
        path.erase(path.begin());
        path = TrimAscii(path);
    }
    if (path.empty()) {
        return false;
    }

    entry.expectedHash = ToLowerAscii(hash);
    entry.relativePath = Utf8ToWide(path);
    return !entry.relativePath.empty();
}

bool LoadMd5Manifest(const std::wstring& path, std::vector<Md5Entry>& entries, std::wstring& error) {
    entries.clear();

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = L"Could not open MD5 manifest: " + path;
        return false;
    }

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart <= 0 ||
        fileSize.QuadPart > static_cast<LONGLONG>(64 * 1024 * 1024)) {
        CloseHandle(file);
        error = L"Invalid MD5 manifest size: " + path;
        return false;
    }

    const HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    CloseHandle(file);
    if (!mapping) {
        error = L"Could not map MD5 manifest: " + path;
        return false;
    }

    const char* view = static_cast<const char*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
    if (!view) {
        CloseHandle(mapping);
        error = L"Could not view MD5 manifest: " + path;
        return false;
    }

    entries.reserve(32000);
    const size_t size = static_cast<size_t>(fileSize.QuadPart);
    size_t lineStart = 0;
    for (size_t i = 0; i < size; ++i) {
        if (view[i] != '\n') {
            continue;
        }

        size_t lineEnd = i;
        if (lineEnd > lineStart && view[lineEnd - 1] == '\r') {
            --lineEnd;
        }
        if (lineEnd > lineStart) {
            Md5Entry entry;
            if (ParseMd5Line(std::string(view + lineStart, lineEnd - lineStart), entry)) {
                entries.push_back(std::move(entry));
            }
        }
        lineStart = i + 1;
    }

    if (lineStart < size) {
        size_t lineEnd = size;
        if (lineEnd > lineStart && view[lineEnd - 1] == '\r') {
            --lineEnd;
        }
        Md5Entry entry;
        if (ParseMd5Line(std::string(view + lineStart, lineEnd - lineStart), entry)) {
            entries.push_back(std::move(entry));
        }
    }

    UnmapViewOfFile(view);
    CloseHandle(mapping);

    if (entries.empty()) {
        error = L"MD5 manifest contained no file entries: " + path;
        return false;
    }

    return true;
}

bool WriteFailedFilesList(const std::wstring& path, const std::vector<std::wstring>& failures) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    for (const auto& failure : failures) {
        const std::string line = WideToUtf8(failure);
        out.write(line.data(), static_cast<std::streamsize>(line.size()));
        out.put('\n');
    }
    return out.good();
}

void ReportProgress(const VerifyOptions& options, size_t current, size_t total, const std::wstring& file,
                    size_t& lastPercent) {
    if (!options.onProgress || total == 0) {
        return;
    }

    const size_t percent = (current * 100) / total;
    if (percent == lastPercent && current != total) {
        return;
    }
    lastPercent = percent;

    VerifyProgress progress;
    progress.current = current;
    progress.total = total;
    progress.file = file;
    options.onProgress(progress);
}

void ReportProgressAtomic(const VerifyOptions& options, std::atomic<size_t>& completed, size_t total,
                          const std::wstring& file, std::atomic<size_t>& lastReportedCount) {
    if (!options.onProgress || total == 0) {
        return;
    }

    const size_t current = completed.load(std::memory_order_relaxed);
    if (current == 0) {
        return;
    }

    size_t previous = lastReportedCount.load(std::memory_order_relaxed);
    const bool finished = current == total;
    if (!finished && current > previous && current - previous < kProgressReportInterval) {
        return;
    }
    if (!finished && current <= previous) {
        return;
    }

    while (!lastReportedCount.compare_exchange_weak(previous, current, std::memory_order_relaxed,
                                                     std::memory_order_relaxed)) {
        if (!finished && current > previous && current - previous < kProgressReportInterval) {
            return;
        }
        if (!finished && current <= previous) {
            return;
        }
    }

    VerifyProgress progress;
    progress.current = current;
    progress.total = total;
    progress.file = file;
    options.onProgress(progress);
}

unsigned ChooseVerifyThreadCount(const std::wstring& driveRoot, const size_t requested) {
    if (requested > 0) {
        return static_cast<unsigned>(std::min(requested, static_cast<size_t>(32)));
    }

    unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0) {
        hw = 4;
    }
    hw = std::max(2u, std::min(hw, 8u));

    if (driveRoot.size() >= 2 && driveRoot[1] == L':') {
        const wchar_t root[] = {driveRoot[0], L':', L'\\', L'\0'};
        if (GetDriveTypeW(root) == DRIVE_REMOVABLE) {
            // USB flash drives bottleneck on I/O; extra threads mostly add seek contention.
            hw = std::min(hw, 2u);
        }
    }

    return hw;
}

enum class EntryVerifyStatus { Verified, Missing, HashError, HashMismatch };

std::wstring FormatCheckLogLine(const EntryVerifyStatus status, const std::wstring& relativePath) {
    switch (status) {
        case EntryVerifyStatus::Verified:
            return L"OK  " + relativePath;
        case EntryVerifyStatus::Missing:
            return L"FAIL  " + relativePath + L"  missing";
        case EntryVerifyStatus::HashError:
            return L"FAIL  " + relativePath + L"  hash error";
        case EntryVerifyStatus::HashMismatch:
            return L"FAIL  " + relativePath + L"  hash mismatch";
    }
    return relativePath;
}

std::wstring FormatSkipLogLine(const std::wstring& relativePath) {
    return L"SKIP  " + relativePath;
}

void NotifyFileLog(const VerifyOptions& options, const size_t current, const size_t total,
                   const std::wstring& line) {
    if (!options.onFileLog || total == 0) {
        return;
    }

    VerifyFileLogEntry entry;
    entry.current = current;
    entry.total = total;
    entry.line = line;
    options.onFileLog(entry);
}

class VerifyCheckLog {
public:
    bool Open(const std::wstring& path, const std::wstring& driveRoot, const std::wstring& manifestPath) {
        path_ = path;
        file_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) {
            path_.clear();
            return false;
        }

        std::string header = "===== MediCat verify check log =====\r\n";
        header += "Drive: " + WideToUtf8(driveRoot) + "\r\n";
        header += "Manifest: " + WideToUtf8(manifestPath) + "\r\n\r\n";
        WriteLocked(header.data(), header.size());
        return true;
    }

    bool IsOpen() const { return file_ != INVALID_HANDLE_VALUE; }

    void WriteEntry(const EntryVerifyStatus status, const std::wstring& relativePath) {
        if (file_ == INVALID_HANDLE_VALUE) {
            return;
        }
        WriteLine(FormatCheckLogLine(status, relativePath));
    }

    void WriteSkipped(const std::wstring& relativePath) {
        if (file_ == INVALID_HANDLE_VALUE) {
            return;
        }
        WriteLine(FormatSkipLogLine(relativePath));
    }

    void WriteSummary(const std::wstring& summary) {
        if (file_ == INVALID_HANDLE_VALUE) {
            return;
        }
        WriteLocked("\r\n", 2);
        WriteLine(summary);
    }

    void Close() {
        std::lock_guard lock(mutex_);
        if (file_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_);
            file_ = INVALID_HANDLE_VALUE;
        }
    }

    const std::wstring& Path() const { return path_; }

private:
    void WriteLine(const std::wstring& line) {
        const std::string utf8 = WideToUtf8(line);
        std::lock_guard lock(mutex_);
        WriteLocked(utf8.data(), utf8.size());
        WriteLocked("\r\n", 2);
        FlushLocked();
    }

    void WriteLocked(const char* data, const size_t len) {
        if (file_ == INVALID_HANDLE_VALUE || !data || len == 0) {
            return;
        }
        DWORD written = 0;
        WriteFile(file_, data, static_cast<DWORD>(len), &written, nullptr);
    }

    void FlushLocked() {
        if (file_ != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(file_);
        }
    }

    std::wstring path_;
    HANDLE file_ = INVALID_HANDLE_VALUE;
    std::mutex mutex_;
};

EntryVerifyStatus VerifyEntry(const std::wstring& driveRoot, const Md5Entry& entry, std::wstring& failureDetail,
                              uint64_t& bytesRead) {
    bytesRead = 0;
    const std::wstring filePath = ResolveVerifyPath(driveRoot, entry.relativePath);

    if (!FileExists(filePath)) {
        failureDetail = entry.relativePath + L" (missing)";
        return EntryVerifyStatus::Missing;
    }

    std::string actualHash;
    std::wstring hashError;
    if (!ComputeFileMd5(filePath, actualHash, hashError, &bytesRead)) {
        failureDetail = entry.relativePath + L" (hash error: " + hashError + L")";
        return EntryVerifyStatus::HashError;
    }

    if (actualHash == entry.expectedHash) {
        return EntryVerifyStatus::Verified;
    }

    failureDetail = entry.relativePath + L" (hash mismatch)";
    return EntryVerifyStatus::HashMismatch;
}

struct ParallelVerifyState {
    std::mutex mutex;
    size_t verifiedFiles = 0;
    size_t failedFiles = 0;
    size_t missingFiles = 0;
    size_t hashMismatchFiles = 0;
    size_t loggedFailures = 0;
    std::vector<std::wstring> failures;
    std::atomic<size_t> completed{0};
    std::atomic<size_t> lastReportedCount{0};
    std::atomic<uint64_t> bytesHashed{0};
};

void RecordFailure(const VerifyOptions& options, ParallelVerifyState& state, const std::wstring& relativePath,
                   const std::wstring& failureDetail, const wchar_t* logSuffix) {
    std::lock_guard lock(state.mutex);
    ++state.failedFiles;
    if (state.failures.size() < kMaxStoredFailures) {
        state.failures.push_back(failureDetail);
    }
    if (options.onLog && state.loggedFailures < 25) {
        ++state.loggedFailures;
        options.onLog(L"[FAIL] " + relativePath + logSuffix);
    }
}

void RecordSuccess(ParallelVerifyState& state, const uint64_t bytesRead) {
    state.bytesHashed.fetch_add(bytesRead, std::memory_order_relaxed);
    std::lock_guard lock(state.mutex);
    ++state.verifiedFiles;
}

void RecordMissing(ParallelVerifyState& state) {
    std::lock_guard lock(state.mutex);
    ++state.missingFiles;
}

void RecordHashMismatch(ParallelVerifyState& state) {
    std::lock_guard lock(state.mutex);
    ++state.hashMismatchFiles;
}

void VerifyEntriesParallel(const VerifyOptions& options, const std::wstring& driveRoot,
                           const std::wstring& manifestPath, const std::vector<Md5Entry>& entries,
                           VerifyResult& result) {
    const size_t skippedFiles = static_cast<size_t>(
        std::count_if(entries.begin(), entries.end(),
                      [](const Md5Entry& entry) { return ShouldSkipKnownFile(entry.relativePath); }));
    result.totalFiles = entries.size() - skippedFiles;
    result.skippedFiles = skippedFiles;

    if (result.totalFiles == 0) {
        return;
    }

    const unsigned threadCount = ChooseVerifyThreadCount(driveRoot, options.threadCount);
    if (options.onLog) {
        options.onLog(L"Verifying with " + std::to_wstring(threadCount) + L" threads");
    }

    VerifyCheckLog checkLog;
    if (!options.checkLogPath.empty()) {
        if (checkLog.Open(options.checkLogPath, driveRoot, manifestPath)) {
            result.checkLogPath = checkLog.Path();
            if (options.onLog) {
                options.onLog(L"Writing per-file verify log to " + result.checkLogPath);
            }
        } else if (options.onLog) {
            options.onLog(L"Could not open verify log: " + options.checkLogPath);
        }
    }

    ParallelVerifyState state;
    std::atomic<size_t> nextIndex{0};
    const auto startedAt = std::chrono::steady_clock::now();

    const auto worker = [&]() {
        while (true) {
            if (IsCancelRequested()) {
                break;
            }

            const size_t index = nextIndex.fetch_add(1, std::memory_order_relaxed);
            if (index >= entries.size()) {
                break;
            }

            const Md5Entry& entry = entries[index];
            if (ShouldSkipKnownFile(entry.relativePath)) {
                if (checkLog.IsOpen()) {
                    checkLog.WriteSkipped(entry.relativePath);
                }
                NotifyFileLog(options, state.completed.load(), result.totalFiles,
                              FormatSkipLogLine(entry.relativePath));
                continue;
            }

            std::wstring failureDetail;
            uint64_t bytesRead = 0;
            const EntryVerifyStatus status = VerifyEntry(driveRoot, entry, failureDetail, bytesRead);
            if (checkLog.IsOpen()) {
                checkLog.WriteEntry(status, entry.relativePath);
            }

            switch (status) {
                case EntryVerifyStatus::Verified:
                    RecordSuccess(state, bytesRead);
                    break;
                case EntryVerifyStatus::Missing:
                    RecordMissing(state);
                    RecordFailure(options, state, entry.relativePath, failureDetail, L" - File not found");
                    break;
                case EntryVerifyStatus::HashError:
                    RecordFailure(options, state, entry.relativePath, failureDetail, L" - Hash error");
                    break;
                case EntryVerifyStatus::HashMismatch:
                    RecordHashMismatch(state);
                    RecordFailure(options, state, entry.relativePath, failureDetail, L" - Hash mismatch");
                    break;
            }

            state.completed.fetch_add(1, std::memory_order_relaxed);
            NotifyFileLog(options, state.completed.load(), result.totalFiles,
                          FormatCheckLogLine(status, entry.relativePath));
            ReportProgressAtomic(options, state.completed, result.totalFiles, entry.relativePath,
                                 state.lastReportedCount);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (unsigned i = 0; i < threadCount; ++i) {
        threads.emplace_back(worker);
    }
    for (std::thread& thread : threads) {
        thread.join();
    }

    if (IsCancelRequested()) {
        result.error = L"Cancelled";
        result.success = false;
        if (checkLog.IsOpen()) {
            checkLog.WriteSummary(L"Cancelled");
            checkLog.Close();
        }
        return;
    }

    result.verifiedFiles = state.verifiedFiles;
    result.failedFiles = state.failedFiles;
    result.missingFiles = state.missingFiles;
    result.hashMismatchFiles = state.hashMismatchFiles;
    result.failures = std::move(state.failures);

    const size_t processed = result.verifiedFiles + result.failedFiles;
    if (processed != result.totalFiles) {
        result.error = L"Verification incomplete: processed " + std::to_wstring(processed) + L" of " +
                       std::to_wstring(result.totalFiles) + L" files";
        result.success = false;
        if (checkLog.IsOpen()) {
            checkLog.WriteSummary(L"Incomplete verification");
            checkLog.Close();
        }
        return;
    }

    if (options.onLog) {
        const auto elapsed = std::chrono::steady_clock::now() - startedAt;
        const double seconds = std::max(0.001, std::chrono::duration<double>(elapsed).count());
        const uint64_t bytesHashed = state.bytesHashed.load(std::memory_order_relaxed);
        const double mbPerSec = (static_cast<double>(bytesHashed) / (1024.0 * 1024.0)) / seconds;

        std::wostringstream throughput;
        throughput.setf(std::ios::fixed);
        throughput.precision(1);
        throughput << mbPerSec << L" MB/s";

        std::wostringstream elapsedSeconds;
        elapsedSeconds.setf(std::ios::fixed);
        elapsedSeconds.precision(1);
        elapsedSeconds << seconds;

        options.onLog(L"Verify summary: " + std::to_wstring(result.verifiedFiles) + L" passed, " +
                      std::to_wstring(result.failedFiles) + L" failed, " + std::to_wstring(result.skippedFiles) +
                      L" skipped");
        options.onLog(L"Hashed " + FormatBytes(bytesHashed) + L" in " + elapsedSeconds.str() + L" s (" +
                      throughput.str() + L")");
        if (mbPerSec > 800.0) {
            options.onLog(L"Note: unusually fast throughput usually means Windows read cache (RAM), not a fresh USB "
                          L"read. Re-run after reboot or eject/reinsert the drive for a cold read test.");
        }
    }

    if (checkLog.IsOpen()) {
        checkLog.WriteSummary(L"Summary: " + std::to_wstring(result.verifiedFiles) + L" passed, " +
                              std::to_wstring(result.failedFiles) + L" failed, " +
                              std::to_wstring(result.skippedFiles) + L" skipped");
        checkLog.Close();
    }

    ReportProgressAtomic(options, state.completed, result.totalFiles, L"", state.lastReportedCount);
}

bool ManifestHasEntries(const std::wstring& path, std::wstring& error) {
    std::vector<Md5Entry> entries;
    if (!LoadMd5Manifest(path, entries, error)) {
        return false;
    }
    return !entries.empty();
}

void AppendManifestCandidates(const std::wstring& directory, std::vector<std::wstring>& candidates) {
    if (directory.empty()) {
        return;
    }

    candidates.push_back(JoinPath(directory, L"MedicatFiles.md5"));
    candidates.push_back(JoinPath(directory, L"hasher\\MedicatFiles.md5"));
    candidates.push_back(JoinPath(directory, L"hasher\\drivefiles.md5"));
}

bool TryParentDirectory(std::wstring& directory) {
    if (directory.empty()) {
        return false;
    }

    wchar_t buffer[MAX_PATH]{};
    wcsncpy_s(buffer, directory.c_str(), _TRUNCATE);
    if (!PathRemoveFileSpecW(buffer) || buffer[0] == L'\0') {
        return false;
    }

    const std::wstring parent(buffer);
    if (parent == directory) {
        return false;
    }

    directory = parent;
    return true;
}

}  // namespace

bool ComputeFileMd5(const std::wstring& path, std::string& outHex, std::wstring& error, uint64_t* outBytesRead) {
    outHex.clear();
    if (outBytesRead) {
        *outBytesRead = 0;
    }

    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    HANDLE file = INVALID_HANDLE_VALUE;

    auto cleanup = [&] {
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
            file = INVALID_HANDLE_VALUE;
        }
        if (hash) {
            CryptDestroyHash(hash);
            hash = 0;
        }
        if (provider) {
            CryptReleaseContext(provider, 0);
            provider = 0;
        }
    };

    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        error = L"CryptAcquireContext failed";
        return false;
    }
    if (!CryptCreateHash(provider, CALG_MD5, 0, 0, &hash)) {
        error = L"CryptCreateHash failed";
        cleanup();
        return false;
    }

    file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = L"Could not open file for hashing";
        cleanup();
        return false;
    }

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart < 0) {
        error = L"Could not determine file size";
        cleanup();
        return false;
    }

    std::vector<BYTE> buffer(static_cast<size_t>(1) << 20);
    uint64_t totalRead = 0;
    DWORD read = 0;
    while (true) {
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr)) {
            const DWORD readError = GetLastError();
            if (readError != ERROR_HANDLE_EOF && readError != ERROR_SUCCESS) {
                error = L"ReadFile failed while hashing";
                cleanup();
                return false;
            }
            break;
        }
        if (read == 0) {
            break;
        }

        if (!CryptHashData(hash, buffer.data(), read, 0)) {
            error = L"CryptHashData failed";
            cleanup();
            return false;
        }
        totalRead += read;
    }

    if (totalRead != static_cast<uint64_t>(fileSize.QuadPart)) {
        error = L"Incomplete file read while hashing (" + std::to_wstring(totalRead) + L" of " +
                std::to_wstring(fileSize.QuadPart) + L" bytes)";
        cleanup();
        return false;
    }

    BYTE digest[16]{};
    DWORD digestLen = sizeof(digest);
    if (!CryptGetHashParam(hash, HP_HASHVAL, digest, &digestLen, 0)) {
        error = L"CryptGetHashParam failed";
        cleanup();
        return false;
    }

    static constexpr char kHex[] = "0123456789abcdef";
    outHex.resize(32);
    for (DWORD i = 0; i < digestLen; ++i) {
        outHex[i * 2] = kHex[(digest[i] >> 4) & 0x0f];
        outHex[i * 2 + 1] = kHex[digest[i] & 0x0f];
    }

    if (outBytesRead) {
        *outBytesRead = totalRead;
    }

    cleanup();
    return true;
}

bool EnsureMedicatMd5Manifest(const std::wstring& installerRoot, const std::wstring& tempDir,
                              std::wstring& manifestPath, std::wstring& error) {
    std::vector<std::wstring> candidates;

    const std::wstring offlineManifest = ResolveOfflineMd5Manifest();
    if (!offlineManifest.empty()) {
        candidates.push_back(offlineManifest);
    }

    AppendManifestCandidates(tempDir, candidates);

    std::wstring walk = installerRoot;
    for (int depth = 0; depth < 8; ++depth) {
        AppendManifestCandidates(walk, candidates);
        if (!TryParentDirectory(walk)) {
            break;
        }
    }

    for (const auto& candidate : candidates) {
        if (!FileExists(candidate)) {
            continue;
        }

        std::wstring parseError;
        if (ManifestHasEntries(candidate, parseError)) {
            manifestPath = candidate;
            return true;
        }
    }

    const std::wstring downloadTargets[] = {
        JoinPath(tempDir, L"MedicatFiles.hasher.md5"),
        JoinPath(tempDir, L"MedicatFiles.md5"),
    };
    const std::wstring downloadUrls[] = {kMd5DownloadUrlHasher, kMd5DownloadUrlPrimary};

    for (size_t i = 0; i < std::size(downloadUrls); ++i) {
        std::wstring downloadError;
        DeleteFileW(downloadTargets[i].c_str());
        if (!HttpDownloadFile(downloadUrls[i], downloadTargets[i], downloadError)) {
            if (error.empty()) {
                error = downloadError;
            }
            continue;
        }

        std::wstring parseError;
        if (ManifestHasEntries(downloadTargets[i], parseError)) {
            manifestPath = downloadTargets[i];
            return true;
        }

        DeleteFileW(downloadTargets[i].c_str());
        if (error.empty()) {
            error = parseError;
        }
    }

    if (error.empty()) {
        error = L"MedicatFiles.md5 was not found locally and no valid manifest could be downloaded";
    }
    return false;
}

MedicatPresenceResult CheckMedicatPresenceOnDrive(const std::wstring& driveRoot) {
    MedicatPresenceResult result;
    result.markersTotal = static_cast<unsigned>(std::size(kMedicatPresenceMarkers));

    for (const MedicatPresenceMarker& marker : kMedicatPresenceMarkers) {
        result.weightTotal += marker.weight;
        const std::wstring path = ResolveVerifyPath(driveRoot, marker.relativePath);
        if (PathMarkerExists(path, marker.isDirectory)) {
            ++result.markersFound;
            result.weightFound += marker.weight;
            result.foundMarkers.push_back(marker.relativePath);
        } else {
            result.missingMarkers.push_back(marker.relativePath);
        }
    }

    if (result.weightTotal > 0) {
        result.scorePercent = (result.weightFound * 100u) / result.weightTotal;
    }
    result.likelyInstalled = result.scorePercent >= kMedicatPresenceProceedThresholdPercent;
    return result;
}

VerifyResult VerifyMedicatFiles(const VerifyOptions& options) {
    VerifyResult result;

    if (options.onLog) {
        options.onLog(L"Resolving MD5 manifest...");
    }

    std::wstring manifestPath;
    std::wstring manifestError;
    if (!options.manifestPath.empty() && FileExists(options.manifestPath)) {
        manifestPath = options.manifestPath;
    } else if (!EnsureMedicatMd5Manifest(options.installerRoot, options.tempDir, manifestPath, manifestError)) {
        result.error = manifestError;
        return result;
    }

    if (options.onLog) {
        options.onLog(L"Loading MD5 manifest: " + manifestPath);
    }

    std::vector<Md5Entry> entries;
    if (!LoadMd5Manifest(manifestPath, entries, manifestError)) {
        result.error = manifestError;
        return result;
    }

    if (options.onLog) {
        options.onLog(L"Loaded " + std::to_wstring(entries.size()) + L" manifest entries");
    }

    std::wstring driveRoot = options.driveRoot;
    if (driveRoot.size() == 2 && driveRoot[1] == L':') {
        driveRoot += L'\\';
    }

    VerifyEntriesParallel(options, driveRoot, manifestPath, entries, result);

    if (result.failedFiles > 0 && !options.failedListPath.empty()) {
        if (WriteFailedFilesList(options.failedListPath, result.failures)) {
            result.failedListPath = options.failedListPath;
        }
    }

    result.success = result.error.empty() && result.failedFiles == 0;
    return result;
}

}  // namespace medicat
