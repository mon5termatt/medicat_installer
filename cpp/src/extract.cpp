#include "extract.h"

#include "drives.h"
#include "util.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <sstream>
#include <thread>

namespace medicat {

namespace {

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

}  // namespace

ExtractResult Extract7zArchive(
    const std::wstring& sevenZipExe,
    const std::wstring& archivePath,
    const std::wstring& destinationRoot,
    const uint64_t totalUncompressedBytes,
    const uint64_t initialFreeBytes,
    ProgressCallback onProgress) {
    ExtractResult result;

    std::wstring dest = destinationRoot;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }
    CreateDirectoryW(dest.c_str(), nullptr);

    std::wstring cmd = L"\"" + sevenZipExe + L"\" x -bsp1 -bso1 -bse1 -o\"" + dest + L"\" \"" +
                       archivePath + L"\" -aoa -y";

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

    std::atomic<bool> running{true};
    std::wstring lastFile;
    int lastPercent = -1;
    std::wstring stderrText;

    std::thread stderrThread([&] {
        char buf[4096];
        DWORD n = 0;
        while (running.load()) {
            if (!ReadFile(stderrRead, buf, sizeof(buf), &n, nullptr) || n == 0) {
                break;
            }
            const int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(n), nullptr, 0);
            std::wstring chunk(static_cast<size_t>(wlen), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(n), chunk.data(), wlen);
            stderrText += chunk;
        }
    });

    auto lastPoll = std::chrono::steady_clock::now();
    std::wstring lineBuffer;

    while (true) {
        DWORD avail = 0;
        if (!PeekNamedPipe(stdoutRead, nullptr, 0, nullptr, &avail, nullptr)) {
            break;
        }

        if (avail > 0) {
            std::string chunk(avail, '\0');
            DWORD read = 0;
            if (ReadFile(stdoutRead, chunk.data(), avail, &read, nullptr) && read > 0) {
                chunk.resize(read);
                const int wlen =
                    MultiByteToWideChar(CP_UTF8, 0, chunk.data(), static_cast<int>(read), nullptr, 0);
                std::wstring wide(static_cast<size_t>(wlen), L'\0');
                MultiByteToWideChar(CP_UTF8, 0, chunk.data(), static_cast<int>(read), wide.data(), wlen);

                for (wchar_t ch : wide) {
                    if (ch == L'\r' || ch == L'\n') {
                        int pct = -1;
                        std::wstring file;
                        if (Parse7zLine(lineBuffer, pct, file)) {
                            if (!file.empty()) {
                                lastFile = file;
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
        }

        const auto now = std::chrono::steady_clock::now();
        if (onProgress &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPoll).count() >= 500) {
            lastPoll = now;
            uint64_t written = 0;
            if (initialFreeBytes > 0) {
                const uint64_t currentFree = GetDriveFreeBytes(dest);
                if (currentFree < initialFreeBytes) {
                    written = initialFreeBytes - currentFree;
                }
            }

            ExtractProgress progress;
            progress.bytesWritten = written;
            progress.totalBytes = totalUncompressedBytes;
            progress.file = lastFile;
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

    running = false;
    if (stderrThread.joinable()) {
        stderrThread.join();
    }

    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

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
        std::wostringstream err;
        err << L"7za exited with code " << exitCode;
        if (!stderrText.empty()) {
            err << L": " << stderrText.substr(0, 500);
        }
        result.error = err.str();
    }

    return result;
}

}  // namespace medicat
