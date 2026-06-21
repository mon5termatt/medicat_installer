#include "extract.h"

#include "cancel.h"
#include "drives.h"
#include "i18n.h"
#include "util.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace medicat {

bool IsBannerLine(const std::wstring& line) {
    if (line.empty()) {
        return true;
    }
    static const wchar_t* kSkip[] = {
        L"7-Zip", L"7-Zi", L"p7zip", L"Copyright", L"Scanning", L"Extracting archive:",
        L"Everything is Ok", L"Archives:", L"Folders:", L"Files:", L"ERROR:", L"Open ERROR:",
        L"System ERROR:", L"Type =", L"Method =", L"Solid =", L"Blocks =", L"Physical Size =",
        L"Headers Size =", L"Path =",
    };
    for (const wchar_t* prefix : kSkip) {
        if (line.rfind(prefix, 0) == 0) {
            return true;
        }
    }
    if (line.find(L"----") == 0) {
        return true;
    }
    return false;
}

bool LooksLikeFilePath(const std::wstring& line) {
    return line.find(L'\\') != std::wstring::npos || line.find(L'/') != std::wstring::npos;
}

bool Parse7zLine(const std::wstring& line, int& percent, std::wstring& file) {
    percent = -1;
    file.clear();
    const std::wstring trimmed = [&] {
        size_t a = line.find_first_not_of(L" \t");
        if (a == std::wstring::npos) {
            return std::wstring{};
        }
        size_t b = line.find_last_not_of(L" \t");
        return line.substr(a, b - a + 1);
    }();

    if (trimmed.empty() || IsBannerLine(trimmed)) {
        return false;
    }

    const size_t pctPos = trimmed.find(L'%');
    if (pctPos != std::wstring::npos) {
        const std::wstring num = trimmed.substr(0, pctPos);
        percent = _wtoi(num.c_str());
        const size_t dash = trimmed.find(L" - ", pctPos);
        if (dash != std::wstring::npos && dash + 3 < trimmed.size()) {
            file = trimmed.substr(dash + 3);
        }
        return true;
    }

    if (trimmed.rfind(L"- ", 0) == 0 && trimmed.size() > 2) {
        file = trimmed.substr(2);
        return LooksLikeFilePath(file);
    }

    if (LooksLikeFilePath(trimmed)) {
        file = trimmed;
        return true;
    }
    return false;
}

int PercentFromBytes(uint64_t written, uint64_t total) {
    if (total == 0) {
        return 0;
    }
    const uint64_t pct = (written * 100) / total;
    if (pct > 99) {
        return 99;
    }
    return static_cast<int>(pct);
}

bool WriteUtf8ListFile(const std::wstring& path, const std::vector<std::wstring>& lines) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    std::string out;
    out.reserve(lines.size() * 64);
    for (const auto& line : lines) {
        out += WideToUtf8(line);
        out += "\r\n";
    }
    DWORD written = 0;
    const BOOL ok = out.empty() ? TRUE : WriteFile(file, out.data(), static_cast<DWORD>(out.size()), &written, nullptr);
    CloseHandle(file);
    return ok != FALSE;
}

class RawLogTee {
public:
    bool Open(const std::wstring& path, const std::wstring& commandLine) {
        file_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) {
            return false;
        }

        std::string header = "===== 7za extract log =====\r\n";
        header += WideToUtf8(commandLine);
        header += "\r\n\r\n";
        WriteLocked(header.data(), header.size());
        return true;
    }

    void WriteStdout(const char* data, const size_t len) { Write(data, len, false); }

    void WriteStderr(const char* data, const size_t len) { Write(data, len, true); }

    void Flush() {
        std::lock_guard lock(mutex_);
        FlushLocked();
    }

    void Close() {
        std::lock_guard lock(mutex_);
        FlushLocked();
        if (file_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_);
            file_ = INVALID_HANDLE_VALUE;
        }
    }

private:
    void Write(const char* data, const size_t len, const bool isStderr) {
        if (!data || len == 0) {
            return;
        }
        std::lock_guard lock(mutex_);
        if (file_ == INVALID_HANDLE_VALUE) {
            return;
        }
        if (isStderr && !stderrMarkerWritten_) {
            static constexpr char kMarker[] = "\r\n----- stderr -----\r\n";
            WriteLocked(kMarker, sizeof(kMarker) - 1);
            stderrMarkerWritten_ = true;
        }
        pending_.append(data, len);
        if (pending_.size() >= 65536) {
            FlushLocked();
        }
    }

    void WriteLocked(const char* data, const size_t len) {
        if (file_ == INVALID_HANDLE_VALUE || !data || len == 0) {
            return;
        }
        DWORD written = 0;
        WriteFile(file_, data, static_cast<DWORD>(len), &written, nullptr);
    }

    void FlushLocked() {
        if (!pending_.empty()) {
            WriteLocked(pending_.data(), pending_.size());
            pending_.clear();
        }
    }

    HANDLE file_ = INVALID_HANDLE_VALUE;
    std::mutex mutex_;
    std::string pending_;
    bool stderrMarkerWritten_ = false;
};

void ProcessStdoutChunk(const std::string& chunk, std::wstring& lineBuffer, std::wstring& lastFile, int& lastPercent,
                        const uint64_t totalUncompressedBytes, const ProgressCallback& onProgress) {
    const int wlen = MultiByteToWideChar(CP_UTF8, 0, chunk.data(), static_cast<int>(chunk.size()), nullptr, 0);
    if (wlen <= 0) {
        return;
    }
    std::wstring wide(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, chunk.data(), static_cast<int>(chunk.size()), wide.data(), wlen);

    for (wchar_t ch : wide) {
        if (ch == L'\r' || ch == L'\n') {
            int pct = -1;
            std::wstring file;
            if (Parse7zLine(lineBuffer, pct, file)) {
                if (!file.empty() && file != lastFile) {
                    lastFile = file;
                    if (onProgress) {
                        ExtractProgress fileProgress;
                        fileProgress.file = file;
                        fileProgress.percent = lastPercent >= 0 ? lastPercent : 0;
                        fileProgress.bytesWritten = 0;
                        fileProgress.totalBytes = totalUncompressedBytes;
                        onProgress(fileProgress);
                    }
                }
                if (pct >= 0) {
                    lastPercent = pct;
                }
            }
            lineBuffer.clear();
        } else if (ch != L'\b') {
            lineBuffer.push_back(ch);
        }
    }
}

void DrainStdout(HANDLE stdoutRead, std::wstring& lineBuffer, std::wstring& lastFile, int& lastPercent,
                 const uint64_t totalUncompressedBytes, const ProgressCallback& onProgress, RawLogTee* logTee) {
    for (;;) {
        char buf[4096];
        DWORD read = 0;
        if (!ReadFile(stdoutRead, buf, sizeof(buf), &read, nullptr) || read == 0) {
            break;
        }
        if (logTee) {
            logTee->WriteStdout(buf, read);
        }
        ProcessStdoutChunk(std::string(buf, read), lineBuffer, lastFile, lastPercent, totalUncompressedBytes,
                           onProgress);
    }

    if (!lineBuffer.empty()) {
        int pct = -1;
        std::wstring file;
        if (Parse7zLine(lineBuffer, pct, file)) {
            if (!file.empty() && file != lastFile && onProgress) {
                ExtractProgress fileProgress;
                fileProgress.file = file;
                fileProgress.percent = lastPercent >= 0 ? lastPercent : 0;
                fileProgress.bytesWritten = 0;
                fileProgress.totalBytes = totalUncompressedBytes;
                onProgress(fileProgress);
            }
        }
        lineBuffer.clear();
    }
}

std::wstring TrimWide(const std::wstring& text) {
    const size_t start = text.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) {
        return {};
    }
    const size_t end = text.find_last_not_of(L" \t\r\n");
    return text.substr(start, end - start + 1);
}

bool IsUsefulErrorLine(const std::wstring& line) {
    if (line.empty()) {
        return false;
    }
    std::wstring upper = line;
    for (wchar_t& ch : upper) {
        if (ch >= L'a' && ch <= L'z') {
            ch = static_cast<wchar_t>(ch - L'a' + L'A');
        }
    }
    static const wchar_t* kMarkers[] = {
        L"ERROR:", L"OPEN ERROR:", L"SYSTEM ERROR:", L"FATAL ERROR", L"CANNOT OPEN", L"CANNOT CREATE",
        L"CANNOT WRITE", L"CANNOT OPEN OUTPUT", L"WRONG PASSWORD", L"NOT ENOUGH SPACE", L"DISK FULL",
        L"FILE NOT FOUND", L"DATA ERROR", L"UNSUPPORTED", L"DEVICE IS NOT READY", L"I/O DEVICE",
        L"WRITE PROTECTED", L"MEDIA IS WRITE", L"READ ERROR", L"WRITE ERROR", L"ERRORS:",
    };
    for (const wchar_t* marker : kMarkers) {
        if (upper.find(marker) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

std::wstring SummarizeProcessOutput(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }

    std::vector<std::wstring> useful;
    useful.reserve(8);
    std::wstring line;
    for (const wchar_t ch : text) {
        if (ch == L'\r') {
            continue;
        }
        if (ch == L'\n') {
            const std::wstring trimmed = TrimWide(line);
            line.clear();
            if (trimmed.empty()) {
                continue;
            }
            if (IsUsefulErrorLine(trimmed)) {
                if (useful.empty() || useful.back() != trimmed) {
                    useful.push_back(trimmed);
                }
            }
            continue;
        }
        line.push_back(ch);
    }
    const std::wstring trimmed = TrimWide(line);
    if (!trimmed.empty() && IsUsefulErrorLine(trimmed)) {
        if (useful.empty() || useful.back() != trimmed) {
            useful.push_back(trimmed);
        }
    }

    if (useful.empty()) {
        const std::wstring compact = TrimWide(text);
        if (compact.size() <= 400) {
            return compact;
        }
        return compact.substr(0, 400) + L"...";
    }

    if (useful.size() > 6) {
        useful.erase(useful.begin(), useful.end() - 6);
    }

    std::wstring out;
    for (size_t i = 0; i < useful.size(); ++i) {
        if (i > 0) {
            out += L"\n";
        }
        out += useful[i];
    }
    if (out.size() > 500) {
        out.resize(500);
        out += L"...";
    }
    return out;
}

bool ContainsFatalExtractOutput(const std::wstring& text) {
    if (text.empty()) {
        return false;
    }

    std::wstring line;
    for (const wchar_t ch : text) {
        if (ch == L'\r') {
            continue;
        }
        if (ch == L'\n') {
            if (IsUsefulErrorLine(TrimWide(line))) {
                return true;
            }
            line.clear();
            continue;
        }
        line.push_back(ch);
    }
    return IsUsefulErrorLine(TrimWide(line));
}

void SetExtractFailure(ExtractResult& result, const DWORD exitCode, const std::wstring& stderrText) {
    result.exitCode = static_cast<int>(exitCode);
    result.success = false;
    result.detail = SummarizeProcessOutput(stderrText);

    std::wostringstream err;
    err << L"7za exited with code " << exitCode;
    if (!stderrText.empty()) {
        err << L": " << stderrText.substr(0, 500);
    }
    result.error = err.str();
}

std::wstring FormatExtractFailureMessage(const ExtractResult& result) {
    std::wstring message = i18n::Tr(L"messages.process_exit_code", L"7za", std::to_wstring(result.exitCode));
    if (!result.detail.empty()) {
        message += L"\n\n" + result.detail;
    }
    return message;
}

struct ExtractMonitorState {
    bool driveRemoved = false;
    bool ioError = false;
    std::wstring ioDetail;
};

bool PollExtractHealth(const std::wstring& destinationRoot, const std::wstring& stderrText,
                       ExtractMonitorState& state, const bool monitorDestinationDrive) {
    if (monitorDestinationDrive) {
        const DestinationDriveStatus status = CheckDestinationDrive(destinationRoot, &state.ioDetail);
        if (status == DestinationDriveStatus::Removed) {
            state.driveRemoved = true;
            return true;
        }
        if (status == DestinationDriveStatus::IoError) {
            state.ioError = true;
            return true;
        }
    }
    if (ContainsFatalExtractOutput(stderrText)) {
        state.ioError = true;
        state.ioDetail = SummarizeProcessOutput(stderrText);
        return true;
    }
    return false;
}

void ApplyExtractMonitorFailure(ExtractResult& result, const ExtractMonitorState& state, const DWORD exitCode) {
    result.success = false;
    result.exitCode = static_cast<int>(exitCode);
    if (state.driveRemoved) {
        result.driveRemoved = true;
        result.error = L"Drive removed during extraction";
        return;
    }
    if (state.ioError) {
        result.ioError = true;
        result.detail = state.ioDetail;
        result.error = L"Drive I/O error during extraction";
    }
}

ExtractResult Extract7zArchive(
    const std::wstring& sevenZipExe,
    const std::wstring& archivePath,
    const std::wstring& destinationRoot,
    const uint64_t totalUncompressedBytes,
    const uint64_t initialFreeBytes,
    ProgressCallback onProgress,
    const std::wstring& logFilePath,
    const bool monitorDestinationDrive) {
    ExtractResult result;

    std::wstring dest = destinationRoot;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }
    CreateDirectoryW(dest.c_str(), nullptr);

    std::wstring cmd = L"\"" + sevenZipExe + L"\" x -bsp1 -bso1 -bse1 -bb1 -o\"" + dest + L"\" \"" +
                       archivePath + L"\" -aoa -y";

    RawLogTee logTee;
    const bool logging = !logFilePath.empty() && logTee.Open(logFilePath, cmd);

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        result.error = L"Failed to create stdout pipe";
        return result;
    }
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    HANDLE stderrRead = nullptr;
    HANDLE stderrWrite = nullptr;
    if (!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        result.error = L"Failed to create stderr pipe";
        return result;
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
    std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
    cmdLine.push_back(L'\0');

    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);
        result.error = L"Failed to start 7za.exe";
        return result;
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    ChildProcessRegistration childProcess(pi.hProcess);
    std::atomic<bool> running{true};
    std::wstring lastFile;
    int lastPercent = -1;
    std::wstring stderrText;
    std::mutex stderrMutex;

    std::thread stderrThread([&] {
        char buf[4096];
        DWORD n = 0;
        while (running.load()) {
            if (!ReadFile(stderrRead, buf, sizeof(buf), &n, nullptr) || n == 0) {
                break;
            }
            if (logging) {
                logTee.WriteStderr(buf, n);
            }
            const int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(n), nullptr, 0);
            std::wstring chunk(static_cast<size_t>(wlen), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(n), chunk.data(), wlen);
            std::lock_guard lock(stderrMutex);
            stderrText += chunk;
        }
    });

    RawLogTee* logPtr = logging ? &logTee : nullptr;
    auto lastPoll = std::chrono::steady_clock::now();
    std::wstring lineBuffer;
    ExtractMonitorState monitor;

    while (true) {
        if (IsCancelRequested()) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }

        std::wstring stderrSnapshot;
        {
            std::lock_guard lock(stderrMutex);
            stderrSnapshot = stderrText;
        }
        if (PollExtractHealth(dest, stderrSnapshot, monitor, monitorDestinationDrive)) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }

        DWORD avail = 0;
        if (!PeekNamedPipe(stdoutRead, nullptr, 0, nullptr, &avail, nullptr)) {
            break;
        }

        if (avail > 0) {
            std::string chunk(avail, '\0');
            DWORD read = 0;
            if (ReadFile(stdoutRead, chunk.data(), avail, &read, nullptr) && read > 0) {
                chunk.resize(read);
                if (logging) {
                    logTee.WriteStdout(chunk.data(), chunk.size());
                }
                ProcessStdoutChunk(chunk, lineBuffer, lastFile, lastPercent, totalUncompressedBytes, onProgress);
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (onProgress &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPoll).count() >= 500) {
            lastPoll = now;
            uint64_t written = 0;
            if (monitorDestinationDrive && initialFreeBytes > 0) {
                const uint64_t currentFree = GetDriveFreeBytes(dest);
                if (currentFree < initialFreeBytes) {
                    written = initialFreeBytes - currentFree;
                }
            }

            ExtractProgress progress;
            progress.bytesWritten = written;
            progress.totalBytes = totalUncompressedBytes;
            progress.file.clear();
            if (written > 0 && totalUncompressedBytes > 0) {
                progress.percent = PercentFromBytes(written, totalUncompressedBytes);
            } else if (lastPercent >= 0) {
                progress.percent = lastPercent;
            } else {
                progress.percent = 0;
            }
            onProgress(progress);
        }

        DWORD exitCode = STILL_ACTIVE;
        if (!GetExitCodeProcess(pi.hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
            break;
        }
        if (avail == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    DrainStdout(stdoutRead, lineBuffer, lastFile, lastPercent, totalUncompressedBytes, onProgress, logPtr);

    running = false;
    if (stderrThread.joinable()) {
        stderrThread.join();
    }

    if (logging) {
        logTee.Flush();
        logTee.Close();
    }

    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (monitor.driveRemoved || monitor.ioError) {
        ApplyExtractMonitorFailure(result, monitor, exitCode);
        return result;
    }

    if (IsCancelRequested()) {
        result.cancelled = true;
        result.success = false;
        result.exitCode = static_cast<int>(exitCode);
        result.error = L"Cancelled";
        return result;
    }

    result.exitCode = static_cast<int>(exitCode);
    result.success = (exitCode == 0);

    if (onProgress) {
        ExtractProgress done;
        done.percent = result.success ? 100 : (lastPercent >= 0 ? lastPercent : 0);
        done.file = lastFile;
        done.bytesWritten = totalUncompressedBytes;
        done.totalBytes = totalUncompressedBytes;
        onProgress(done);
    }

    if (!result.success) {
        SetExtractFailure(result, exitCode, stderrText);
    }

    return result;
}

ExtractResult Extract7zArchiveSelective(
    const std::wstring& sevenZipExe,
    const std::wstring& archivePath,
    const std::wstring& destinationRoot,
    const std::vector<std::wstring>& relativePaths,
    ProgressCallback onProgress,
    const std::wstring& logFilePath) {
    ExtractResult result;
    if (relativePaths.empty()) {
        result.success = true;
        result.exitCode = 0;
        return result;
    }

    std::wstring dest = destinationRoot;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }
    CreateDirectoryW(dest.c_str(), nullptr);

    const std::wstring listPath = JoinPath(GetMedicatTempDir(), L"reextract_list.txt");
    if (!WriteUtf8ListFile(listPath, relativePaths)) {
        result.error = L"Failed to write re-extract list file";
        return result;
    }

    std::wstring cmd = L"\"" + sevenZipExe + L"\" x -bsp1 -bso1 -bse1 -bb1 -o\"" + dest + L"\" \"" +
                       archivePath + L"\" -aoa -y @\"" + listPath + L"\"";

    // For selective re-extract we don't have accurate size/free-space baselines; rely on 7z percent parsing.
    const uint64_t totalUncompressedBytes = 0;

    RawLogTee logTee;
    const bool logging = !logFilePath.empty() && logTee.Open(logFilePath, cmd);

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        result.error = L"Failed to create stdout pipe";
        return result;
    }
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    HANDLE stderrRead = nullptr;
    HANDLE stderrWrite = nullptr;
    if (!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        result.error = L"Failed to create stderr pipe";
        return result;
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
    std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
    cmdLine.push_back(L'\0');

    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);
        result.error = L"Failed to start 7za.exe";
        return result;
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    ChildProcessRegistration childProcess(pi.hProcess);
    std::atomic<bool> running{true};
    std::wstring lastFile;
    int lastPercent = -1;
    std::wstring stderrText;
    std::mutex stderrMutex;

    std::thread stderrThread([&] {
        char buf[4096];
        DWORD n = 0;
        while (running.load()) {
            if (!ReadFile(stderrRead, buf, sizeof(buf), &n, nullptr) || n == 0) {
                break;
            }
            if (logging) {
                logTee.WriteStderr(buf, n);
            }
            const int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(n), nullptr, 0);
            std::wstring chunk(static_cast<size_t>(wlen), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(n), chunk.data(), wlen);
            std::lock_guard lock(stderrMutex);
            stderrText += chunk;
        }
    });

    RawLogTee* logPtr = logging ? &logTee : nullptr;
    auto lastPoll = std::chrono::steady_clock::now();
    std::wstring lineBuffer;
    ExtractMonitorState monitor;

    while (true) {
        if (IsCancelRequested()) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }

        std::wstring stderrSnapshot;
        {
            std::lock_guard lock(stderrMutex);
            stderrSnapshot = stderrText;
        }
        if (PollExtractHealth(dest, stderrSnapshot, monitor, true)) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }

        DWORD avail = 0;
        if (!PeekNamedPipe(stdoutRead, nullptr, 0, nullptr, &avail, nullptr)) {
            break;
        }

        if (avail > 0) {
            std::string chunk(avail, '\0');
            DWORD read = 0;
            if (ReadFile(stdoutRead, chunk.data(), avail, &read, nullptr) && read > 0) {
                chunk.resize(read);
                if (logging) {
                    logTee.WriteStdout(chunk.data(), chunk.size());
                }
                ProcessStdoutChunk(chunk, lineBuffer, lastFile, lastPercent, totalUncompressedBytes, onProgress);
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (onProgress && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPoll).count() >= 500) {
            lastPoll = now;
            ExtractProgress progress;
            progress.bytesWritten = 0;
            progress.totalBytes = 0;
            progress.file.clear();
            progress.percent = lastPercent >= 0 ? lastPercent : 0;
            onProgress(progress);
        }

        DWORD exitCode = STILL_ACTIVE;
        if (!GetExitCodeProcess(pi.hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
            break;
        }
        if (avail == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    DrainStdout(stdoutRead, lineBuffer, lastFile, lastPercent, totalUncompressedBytes, onProgress, logPtr);

    running = false;
    if (stderrThread.joinable()) {
        stderrThread.join();
    }

    if (logging) {
        logTee.Flush();
        logTee.Close();
    }

    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (monitor.driveRemoved || monitor.ioError) {
        ApplyExtractMonitorFailure(result, monitor, exitCode);
        return result;
    }

    if (IsCancelRequested()) {
        result.cancelled = true;
        result.success = false;
        result.exitCode = static_cast<int>(exitCode);
        result.error = L"Cancelled";
        return result;
    }

    result.exitCode = static_cast<int>(exitCode);
    result.success = (exitCode == 0);

    if (onProgress) {
        ExtractProgress done;
        done.percent = result.success ? 100 : (lastPercent >= 0 ? lastPercent : 0);
        done.file = lastFile;
        done.bytesWritten = 0;
        done.totalBytes = 0;
        onProgress(done);
    }

    if (!result.success) {
        SetExtractFailure(result, exitCode, stderrText);
    }

    return result;
}

}  // namespace medicat
