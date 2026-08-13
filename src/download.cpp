#include "download.h"

#include "cancel.h"
#include "util.h"

#include <windows.h>
#include <winhttp.h>
#include <winioctl.h>

#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace medicat {

namespace {

struct UrlParts {
    bool https = true;
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = 0;
};

bool ParseUrl(const std::wstring& url, UrlParts& parts, std::wstring& error) {
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);

    wchar_t host[256]{};
    wchar_t path[2048]{};
    uc.lpszHostName = host;
    uc.dwHostNameLength = static_cast<DWORD>(std::size(host));
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = static_cast<DWORD>(std::size(path));

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) {
        error = L"Invalid URL";
        return false;
    }

    parts.host = host;
    parts.path = path;
    parts.port = uc.nPort;
    parts.https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    return true;
}

bool ParseContentRange(const wchar_t* value, uint64_t& start, uint64_t& end, uint64_t& total) {
    if (!value || wcsncmp(value, L"bytes ", 6) != 0) {
        return false;
    }

    const wchar_t* cursor = value + 6;
    wchar_t* endPtr = nullptr;
    start = _wcstoui64(cursor, &endPtr, 10);
    if (!endPtr || *endPtr != L'-') {
        return false;
    }

    cursor = endPtr + 1;
    end = _wcstoui64(cursor, &endPtr, 10);
    if (!endPtr || *endPtr != L'/') {
        return false;
    }

    cursor = endPtr + 1;
    if (*cursor == L'*') {
        total = 0;
        return true;
    }

    total = _wcstoui64(cursor, nullptr, 10);
    return true;
}

bool ReadContentLengthHeader(HINTERNET request, uint64_t& contentLength) {
    contentLength = 0;
    wchar_t lengthText[32]{};
    DWORD lengthTextSize = sizeof(lengthText);
    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, lengthText,
                            &lengthTextSize, WINHTTP_NO_HEADER_INDEX) &&
        lengthText[0] != L'\0') {
        contentLength = _wcstoui64(lengthText, nullptr, 10);
        return contentLength > 0;
    }
    return false;
}

bool ReadContentRangeHeader(HINTERNET request, uint64_t& start, uint64_t& end, uint64_t& total) {
    wchar_t rangeText[128]{};
    DWORD rangeTextSize = sizeof(rangeText);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_RANGE, WINHTTP_HEADER_NAME_BY_INDEX, rangeText,
                             &rangeTextSize, WINHTTP_NO_HEADER_INDEX)) {
        return false;
    }
    return ParseContentRange(rangeText, start, end, total);
}

void AppendFailedUrl(std::wstring& error, const std::wstring& url) {
    if (url.empty() || error.empty()) {
        return;
    }
    if (error.find(url) != std::wstring::npos) {
        return;
    }
    error += L" — " + url;
}

bool OpenHttpGetRequest(const std::wstring& url, HINTERNET& outRequest, HINTERNET& outConnect,
                        HINTERNET& outSession, uint64_t& contentLength, std::wstring& error) {
    contentLength = 0;
    outRequest = nullptr;
    outConnect = nullptr;
    outSession = nullptr;

    UrlParts parts;
    if (!ParseUrl(url, parts, error)) {
        AppendFailedUrl(error, url);
        return false;
    }

    outSession = WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS, 0);
    if (!outSession) {
        error = L"WinHttpOpen failed";
        AppendFailedUrl(error, url);
        return false;
    }

    WinHttpSetTimeouts(outSession, 30000, 30000, 300000, 1800000);

    outConnect = WinHttpConnect(outSession, parts.host.c_str(), parts.port, 0);
    if (!outConnect) {
        error = L"WinHttpConnect failed";
        AppendFailedUrl(error, url);
        return false;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    outRequest = WinHttpOpenRequest(outConnect, L"GET", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                    WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!outRequest) {
        error = L"WinHttpOpenRequest failed";
        AppendFailedUrl(error, url);
        return false;
    }

    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(outRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));

    const wchar_t* headers = L"User-Agent: MedicatInstaller/1.0\r\nAccept: */*";
    if (!WinHttpSendRequest(outRequest, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(outRequest, nullptr)) {
        error = L"HTTP request failed";
        AppendFailedUrl(error, url);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(outRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
        statusCode < 200 || statusCode >= 300) {
        error = L"HTTP request returned status " + std::to_wstring(statusCode);
        AppendFailedUrl(error, url);
        return false;
    }

    ReadContentLengthHeader(outRequest, contentLength);
    return true;
}

bool OpenHttpDownloadRequest(const std::wstring& url, const uint64_t resumeFrom, HINTERNET& outRequest,
                             HINTERNET& outConnect, HINTERNET& outSession, DWORD& statusCode, uint64_t& totalFileSize,
                             uint64_t& rangeStart, std::wstring& error) {
    statusCode = 0;
    totalFileSize = 0;
    rangeStart = resumeFrom;
    outRequest = nullptr;
    outConnect = nullptr;
    outSession = nullptr;

    UrlParts parts;
    if (!ParseUrl(url, parts, error)) {
        AppendFailedUrl(error, url);
        return false;
    }

    outSession = WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS, 0);
    if (!outSession) {
        error = L"WinHttpOpen failed";
        AppendFailedUrl(error, url);
        return false;
    }

    WinHttpSetTimeouts(outSession, 30000, 30000, 300000, 1800000);

    outConnect = WinHttpConnect(outSession, parts.host.c_str(), parts.port, 0);
    if (!outConnect) {
        error = L"WinHttpConnect failed";
        AppendFailedUrl(error, url);
        return false;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    outRequest = WinHttpOpenRequest(outConnect, L"GET", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                    WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!outRequest) {
        error = L"WinHttpOpenRequest failed";
        AppendFailedUrl(error, url);
        return false;
    }

    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(outRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));

    std::wstring headers = L"User-Agent: MedicatInstaller/1.0\r\nAccept: */*";
    if (resumeFrom > 0) {
        headers += L"\r\nRange: bytes=" + std::to_wstring(resumeFrom) + L"-";
    }

    if (!WinHttpSendRequest(outRequest, headers.c_str(), static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(outRequest, nullptr)) {
        error = L"HTTP request failed";
        AppendFailedUrl(error, url);
        return false;
    }

    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(outRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
        error = L"HTTP request failed";
        AppendFailedUrl(error, url);
        return false;
    }

    if (statusCode == 206) {
        uint64_t rangeEnd = 0;
        if (ReadContentRangeHeader(outRequest, rangeStart, rangeEnd, totalFileSize) && totalFileSize > 0) {
            return true;
        }

        uint64_t responseLength = 0;
        if (ReadContentLengthHeader(outRequest, responseLength) && responseLength > 0) {
            totalFileSize = rangeStart + responseLength;
            return true;
        }

        error = L"Partial response missing size headers";
        AppendFailedUrl(error, url);
        return false;
    }

    if (statusCode == 416) {
        uint64_t rangeEnd = 0;
        ReadContentRangeHeader(outRequest, rangeStart, rangeEnd, totalFileSize);
        return true;
    }

    if (statusCode < 200 || statusCode >= 300) {
        error = L"HTTP request returned status " + std::to_wstring(statusCode);
        AppendFailedUrl(error, url);
        return false;
    }

    rangeStart = 0;
    if (!ReadContentLengthHeader(outRequest, totalFileSize)) {
        totalFileSize = 0;
    }
    return true;
}

void CloseHttpHandles(HINTERNET request, HINTERNET connect, HINTERNET session) {
    if (request) {
        WinHttpCloseHandle(request);
    }
    if (connect) {
        WinHttpCloseHandle(connect);
    }
    if (session) {
        WinHttpCloseHandle(session);
    }
}

bool ReadResponse(HINTERNET request, std::vector<BYTE>& out, std::wstring& error) {
    out.clear();
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(request, &avail)) {
            error = L"WinHttpQueryDataAvailable failed";
            return false;
        }
        if (avail == 0) {
            break;
        }

        const size_t offset = out.size();
        out.resize(offset + avail);
        DWORD read = 0;
        if (!WinHttpReadData(request, out.data() + offset, avail, &read)) {
            error = L"WinHttpReadData failed";
            return false;
        }
        out.resize(offset + read);
    }
    return true;
}

bool HttpRequest(const std::wstring& url, std::vector<BYTE>& body, std::wstring& error) {
    HINTERNET session = nullptr;
    HINTERNET connect = nullptr;
    HINTERNET request = nullptr;
    uint64_t contentLength = 0;
    if (!OpenHttpGetRequest(url, request, connect, session, contentLength, error)) {
        CloseHttpHandles(request, connect, session);
        return false;
    }

    const bool ok = ReadResponse(request, body, error);
    CloseHttpHandles(request, connect, session);
    if (!ok) {
        AppendFailedUrl(error, url);
    }
    return ok;
}

std::wstring BytesToWide(const std::vector<BYTE>& bytes) {
    if (bytes.empty()) {
        return L"";
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(bytes.data()),
                                        static_cast<int>(bytes.size()), nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring out(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(bytes.data()), static_cast<int>(bytes.size()),
                        out.data(), len);
    return out;
}

int HttpPostJsonInternal(const std::wstring& url, const std::string& jsonBody, const std::wstring& bearerToken,
                         std::wstring& error) {
    error.clear();
    UrlParts parts;
    if (!ParseUrl(url, parts, error)) {
        AppendFailedUrl(error, url);
        return 0;
    }

    HINTERNET session = WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        error = L"WinHttpOpen failed";
        AppendFailedUrl(error, url);
        return 0;
    }
    WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);

    HINTERNET connect = WinHttpConnect(session, parts.host.c_str(), parts.port, 0);
    if (!connect) {
        error = L"WinHttpConnect failed";
        AppendFailedUrl(error, url);
        WinHttpCloseHandle(session);
        return 0;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request =
        WinHttpOpenRequest(connect, L"POST", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        error = L"WinHttpOpenRequest failed";
        AppendFailedUrl(error, url);
        CloseHttpHandles(nullptr, connect, session);
        return 0;
    }

    const std::wstring authHeader = L"Authorization: Bearer " + bearerToken;
    const std::wstring contentType = L"Content-Type: application/json";
    WinHttpAddRequestHeaders(request, authHeader.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(request, contentType.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);

    const BOOL sent = WinHttpSendRequest(
        request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        jsonBody.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(jsonBody.data()),
        static_cast<DWORD>(jsonBody.size()), static_cast<DWORD>(jsonBody.size()), 0);
    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        error = L"HTTP POST failed";
        AppendFailedUrl(error, url);
        CloseHttpHandles(request, connect, session);
        return 0;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

    std::vector<BYTE> ignored;
    ReadResponse(request, ignored, error);
    CloseHttpHandles(request, connect, session);
    if (!error.empty()) {
        AppendFailedUrl(error, url);
    }
    return static_cast<int>(statusCode);
}

std::wstring g_aria2cPath;

bool EnsureSparseOutputFile(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD bytesReturned = 0;
    DeviceIoControl(file, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
    CloseHandle(file);
    return true;
}

bool SplitOutputPath(const std::wstring& outputPath, std::wstring& dir, std::wstring& name) {
    const size_t slash = outputPath.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        dir = L".";
        name = outputPath;
        return !name.empty();
    }
    dir = outputPath.substr(0, slash);
    name = outputPath.substr(slash + 1);
    return !dir.empty() && !name.empty();
}

bool CaptureProcessOutput(const std::wstring& commandLine, std::string& output, std::wstring& error) {
    output.clear();
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        error = L"Failed to create pipe";
        return false;
    }
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stdoutWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmdLine(commandLine.begin(), commandLine.end());
    cmdLine.push_back(L'\0');
    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si,
                        &pi)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        error = L"Failed to start aria2c.exe";
        return false;
    }
    CloseHandle(stdoutWrite);
    ChildProcessRegistration childProcess(pi.hProcess);

    char chunk[4096];
    DWORD read = 0;
    while (ReadFile(stdoutRead, chunk, sizeof(chunk), &read, nullptr) && read > 0) {
        output.append(chunk, read);
        if (output.size() > 1024 * 1024) {
            break;
        }
    }

    WaitForSingleObject(pi.hProcess, 15000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stdoutRead);
    if (exitCode != 0 && output.empty()) {
        error = L"aria2c --show-files failed";
        return false;
    }
    return true;
}

bool FindTorrentArchiveIndex(const std::wstring& aria2c, const std::wstring& torrentPath,
                             const std::wstring& archiveName, int& index, std::wstring& error) {
    index = 0;
    const std::wstring cmd =
        L"\"" + aria2c + L"\" --show-files --no-conf=true \"" + torrentPath + L"\"";
    std::string listing;
    if (!CaptureProcessOutput(cmd, listing, error)) {
        return false;
    }

    const std::string needle = WideToUtf8(archiveName);
    std::istringstream stream(listing);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find(needle) == std::string::npos) {
            continue;
        }
        const int parsed = atoi(line.c_str());
        if (parsed > 0) {
            index = parsed;
            return true;
        }
    }

    error = L"Could not find " + archiveName + L" in torrent";
    return false;
}

uint64_t ParseAria2ByteCount(const std::string& token) {
    if (token.empty()) {
        return 0;
    }
    char* end = nullptr;
    const double value = std::strtod(token.c_str(), &end);
    if (end == token.c_str() || value < 0) {
        return 0;
    }
    std::string unit(end);
    while (!unit.empty() && (unit.back() == ' ' || unit.back() == ']' || unit.back() == ')')) {
        unit.pop_back();
    }
    uint64_t mul = 1;
    if (unit == "KiB" || unit == "K" || unit == "k") {
        mul = 1024;
    } else if (unit == "MiB" || unit == "M") {
        mul = 1024ull * 1024;
    } else if (unit == "GiB" || unit == "G") {
        mul = 1024ull * 1024 * 1024;
    } else if (unit == "TiB" || unit == "T") {
        mul = 1024ull * 1024 * 1024 * 1024;
    }
    return static_cast<uint64_t>(value * static_cast<double>(mul) + 0.5);
}

bool ParseAria2ProgressPair(const std::string& line, const size_t slash, uint64_t& downloaded, uint64_t& total) {
    size_t left = slash;
    while (left > 0 && line[left - 1] != ' ' && line[left - 1] != ':') {
        --left;
    }
    size_t right = slash + 1;
    while (right < line.size() && line[right] != ' ' && line[right] != '(' && line[right] != ']' &&
           line[right] != '\t') {
        ++right;
    }
    downloaded = ParseAria2ByteCount(line.substr(left, slash - left));
    total = ParseAria2ByteCount(line.substr(slash + 1, right - slash - 1));
    return total > 0 || downloaded > 0;
}

bool ParseAria2ProgressLine(const std::string& line, uint64_t& downloaded, uint64_t& total) {
    const auto hashPos = line.find("[#");
    if (hashPos != std::string::npos) {
        const auto space = line.find(' ', hashPos + 2);
        if (space != std::string::npos) {
            const auto slash = line.find('/', space + 1);
            if (slash != std::string::npos && ParseAria2ProgressPair(line, slash, downloaded, total)) {
                return true;
            }
        }
    }

    const auto sizePos = line.find("SIZE:");
    if (sizePos == std::string::npos) {
        return false;
    }
    const auto slash = line.find('/', sizePos + 5);
    if (slash == std::string::npos) {
        return false;
    }
    return ParseAria2ProgressPair(line, slash, downloaded, total);
}

void ConsumeAria2Output(std::string& buffer, uint64_t& downloaded, uint64_t& total,
                        const std::function<void(uint64_t, uint64_t)>& onProgress) {
    size_t start = 0;
    for (size_t i = 0; i < buffer.size(); ++i) {
        if (buffer[i] != '\n' && buffer[i] != '\r') {
            continue;
        }
        const std::string line = buffer.substr(start, i - start);
        start = i + 1;
        uint64_t lineDownloaded = 0;
        uint64_t lineTotal = 0;
        if (ParseAria2ProgressLine(line, lineDownloaded, lineTotal)) {
            downloaded = lineDownloaded;
            if (lineTotal > 0) {
                total = lineTotal;
            }
            if (onProgress) {
                onProgress(downloaded, total);
            }
        }
    }
    if (start > 0) {
        buffer.erase(0, start);
    }
}

enum class Aria2DownloadResult { Ok, StartFailed, Failed };

Aria2DownloadResult DownloadFileWithAria2(const std::wstring& aria2c, const std::wstring& url,
                                          const std::wstring& outputPath,
                                          const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
                                          std::wstring& error, const bool bitTorrent, const int selectIndex) {
    std::wstring dir;
    std::wstring name;
    if (!SplitOutputPath(outputPath, dir, name)) {
        error = L"Invalid download path";
        AppendFailedUrl(error, url);
        return Aria2DownloadResult::StartFailed;
    }

    CreateDirectoryW(dir.c_str(), nullptr);
    EnsureSparseOutputFile(outputPath);

    const std::wstring logPath = JoinPath(GetExeDirectory(), L"aria.log");
    DeleteFileW(logPath.c_str());

    std::wstring cmd = L"\"" + aria2c +
                       L"\" --no-conf=true --console-log-level=error --summary-interval=1 "
                       L"--show-console-readout=false --human-readable=false --enable-color=false "
                       L"--allow-overwrite=true --auto-file-renaming=false --continue=true "
                       L"--file-allocation=none --async-dns=false --retry-wait=2 "
                       L"--log=\"" +
                       logPath +
                       L"\" --log-level=notice "
                       L"--user-agent=MedicatInstaller/1.0 --stop-with-process=" +
                       std::to_wstring(GetCurrentProcessId()) + L" --dir=\"" + dir + L"\" ";
    if (bitTorrent) {
        cmd += L"--seed-time=0 --bt-enable-lpd=true --enable-dht=true --enable-peer-exchange=true "
               L"--bt-save-metadata=false --bt-remove-unselected-file=true "
               L"--stream-piece-selector=inorder --timeout=180 --connect-timeout=60 --max-tries=0 ";
        if (selectIndex > 0) {
            const std::wstring indexText = std::to_wstring(selectIndex);
            cmd += L"--select-file=" + indexText + L" --index-out=" + indexText + L"=" + name + L" ";
        }
    } else {
        cmd += L"--max-connection-per-server=16 --split=16 --min-split-size=1M --timeout=60 "
               L"--connect-timeout=30 --max-tries=5 --out=\"" +
               name + L"\" ";
    }
    cmd += L"\"" + url + L"\"";

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        error = L"Failed to create aria2c pipe";
        AppendFailedUrl(error, url);
        return Aria2DownloadResult::StartFailed;
    }
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stdoutWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
    cmdLine.push_back(L'\0');

    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, dir.c_str(), &si,
                        &pi)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        error = L"Failed to start aria2c.exe";
        AppendFailedUrl(error, url);
        return Aria2DownloadResult::StartFailed;
    }

    CloseHandle(stdoutWrite);
    ChildProcessRegistration childProcess(pi.hProcess);

    const uint64_t existing = GetFileSizeBytes(outputPath);
    uint64_t downloaded = existing;
    uint64_t total = 0;
    if (onProgress) {
        onProgress(existing, 0);
    }

    std::string buffer;
    char chunk[4096];
    bool cancelled = false;
    for (;;) {
        if (IsCancelRequested()) {
            TerminateProcess(pi.hProcess, 1);
            cancelled = true;
            break;
        }

        DWORD avail = 0;
        if (PeekNamedPipe(stdoutRead, nullptr, 0, nullptr, &avail, nullptr) && avail > 0) {
            const DWORD toRead = avail > sizeof(chunk) ? sizeof(chunk) : avail;
            DWORD read = 0;
            if (ReadFile(stdoutRead, chunk, toRead, &read, nullptr) && read > 0) {
                buffer.append(chunk, read);
                ConsumeAria2Output(buffer, downloaded, total, onProgress);
            }
        }

        const DWORD wait = WaitForSingleObject(pi.hProcess, 100);
        if (wait == WAIT_OBJECT_0) {
            DWORD read = 0;
            while (ReadFile(stdoutRead, chunk, sizeof(chunk), &read, nullptr) && read > 0) {
                buffer.append(chunk, read);
            }
            ConsumeAria2Output(buffer, downloaded, total, onProgress);
            if (!buffer.empty()) {
                uint64_t lineDownloaded = 0;
                uint64_t lineTotal = 0;
                if (ParseAria2ProgressLine(buffer, lineDownloaded, lineTotal)) {
                    downloaded = lineDownloaded;
                    if (lineTotal > 0) {
                        total = lineTotal;
                    }
                    if (onProgress) {
                        onProgress(downloaded, total);
                    }
                }
            }
            break;
        }
    }

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(stdoutRead);

    if (cancelled || IsCancelRequested()) {
        error = L"Download cancelled";
        AppendFailedUrl(error, url);
        return Aria2DownloadResult::Failed;
    }

    if (exitCode != 0) {
        error = L"aria2c exited with code " + std::to_wstring(exitCode);
        AppendFailedUrl(error, url);
        return Aria2DownloadResult::Failed;
    }

    const uint64_t finalSize = GetFileSizeBytes(outputPath);
    if (finalSize == 0) {
        error = L"Download was empty";
        AppendFailedUrl(error, url);
        return Aria2DownloadResult::Failed;
    }
    if (total > 0 && finalSize < total) {
        error = L"Download incomplete";
        AppendFailedUrl(error, url);
        return Aria2DownloadResult::Failed;
    }
    if (onProgress) {
        onProgress(finalSize, finalSize);
    }
    return Aria2DownloadResult::Ok;
}

bool DownloadFileWithWinHttp(const std::wstring& url, const std::wstring& outputPath,
                             const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
                             std::wstring& error) {
    const uint64_t resumeFrom = GetFileSizeBytes(outputPath);

    HINTERNET session = nullptr;
    HINTERNET connect = nullptr;
    HINTERNET request = nullptr;
    DWORD statusCode = 0;
    uint64_t totalFileSize = 0;
    uint64_t rangeStart = resumeFrom;
    if (!OpenHttpDownloadRequest(url, resumeFrom, request, connect, session, statusCode, totalFileSize, rangeStart,
                                 error)) {
        CloseHttpHandles(request, connect, session);
        return false;
    }

    if (statusCode == 416) {
        CloseHttpHandles(request, connect, session);
        if (resumeFrom > 0 && totalFileSize > 0 && resumeFrom >= totalFileSize) {
            if (onProgress) {
                onProgress(totalFileSize, totalFileSize);
            }
            return true;
        }
        error = L"Server rejected resume request";
        AppendFailedUrl(error, url);
        return false;
    }

    if (statusCode != 200 && statusCode != 206) {
        CloseHttpHandles(request, connect, session);
        error = L"HTTP request returned status " + std::to_wstring(statusCode);
        AppendFailedUrl(error, url);
        return false;
    }

    uint64_t baseOffset = 0;
    const bool appendMode = statusCode == 206 && resumeFrom > 0;
    if (appendMode) {
        baseOffset = rangeStart;
    } else if (statusCode == 200 && resumeFrom > 0) {
        baseOffset = 0;
    }

    const std::ios::openmode mode =
        std::ios::binary | (appendMode ? static_cast<std::ios::openmode>(std::ios::app) : std::ios::trunc);
    std::ofstream out(outputPath, mode);
    if (!out) {
        CloseHttpHandles(request, connect, session);
        error = L"Could not open output file";
        AppendFailedUrl(error, url);
        return false;
    }

    std::vector<BYTE> buffer(256 * 1024);
    uint64_t sessionDownloaded = 0;
    if (onProgress) {
        onProgress(baseOffset, totalFileSize);
    }

    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(request, &avail)) {
            out.close();
            CloseHttpHandles(request, connect, session);
            error = L"WinHttpQueryDataAvailable failed";
            AppendFailedUrl(error, url);
            return false;
        }
        if (avail == 0) {
            break;
        }

        const DWORD chunk = avail > static_cast<DWORD>(buffer.size()) ? static_cast<DWORD>(buffer.size()) : avail;
        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data(), chunk, &read) || read == 0) {
            out.close();
            CloseHttpHandles(request, connect, session);
            error = L"WinHttpReadData failed";
            AppendFailedUrl(error, url);
            return false;
        }

        out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(read));
        if (!out.good()) {
            out.close();
            CloseHttpHandles(request, connect, session);
            error = L"Failed to write output file";
            AppendFailedUrl(error, url);
            return false;
        }

        sessionDownloaded += read;
        if (onProgress) {
            onProgress(baseOffset + sessionDownloaded, totalFileSize);
        }
    }

    out.close();
    CloseHttpHandles(request, connect, session);

    const uint64_t finalSize = GetFileSizeBytes(outputPath);
    if (finalSize == 0) {
        error = L"Download was empty";
        AppendFailedUrl(error, url);
        return false;
    }
    if (totalFileSize > 0 && finalSize < totalFileSize) {
        error = L"Download incomplete";
        AppendFailedUrl(error, url);
        return false;
    }

    return true;
}

}  // namespace

void SetAria2cPath(const std::wstring& path) {
    g_aria2cPath = path;
}

int HttpPostJson(const std::wstring& url, const std::string& jsonBody, const std::wstring& bearerToken,
                 std::wstring& error) {
    return HttpPostJsonInternal(url, jsonBody, bearerToken, error);
}

std::wstring ExtractJsonStringField(const std::string& json, const char* field) {
    const std::string needle = std::string("\"") + field + "\":\"";
    const size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        return {};
    }
    const size_t start = pos + needle.size();
    const size_t end = json.find('"', start);
    if (end == std::string::npos || end <= start) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, json.data() + static_cast<int>(start),
                                        static_cast<int>(end - start), nullptr, 0);
    if (len <= 0) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, json.data() + static_cast<int>(start), static_cast<int>(end - start),
                        wide.data(), len);
    return wide;
}

HttpMultipartResult HttpPostMultipartUpload(const std::wstring& url, const std::wstring& bearerToken,
                                            const std::wstring& sessionId, const std::string& manifestJson,
                                            const std::wstring& zipPath) {
    HttpMultipartResult result;
    if (!FileExists(zipPath)) {
        result.error = L"Upload zip not found";
        return result;
    }

    std::ifstream zipIn(zipPath, std::ios::binary);
    if (!zipIn) {
        result.error = L"Could not read upload zip";
        return result;
    }
    std::vector<char> zipBytes((std::istreambuf_iterator<char>(zipIn)), std::istreambuf_iterator<char>());
    if (zipBytes.empty()) {
        result.error = L"Upload zip is empty";
        return result;
    }

    const std::string boundary = "----MedicatFormBoundary9f2c1a7e";
    const std::string sessionUtf8 = medicat::WideToUtf8(sessionId);
    std::ostringstream body;
    auto writeField = [&](const char* name, const std::string& value) {
        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"" << name << "\"\r\n\r\n";
        body << value << "\r\n";
    };

    writeField("session_id", sessionUtf8);
    writeField("manifest", manifestJson);
    body << "--" << boundary << "\r\n";
    body << "Content-Disposition: form-data; name=\"bundle\"; filename=\"support_upload.zip\"\r\n";
    body << "Content-Type: application/zip\r\n\r\n";
    const std::string prefix = body.str();
    const std::string suffix = "\r\n--" + boundary + "--\r\n";

    std::vector<char> payload;
    payload.reserve(prefix.size() + zipBytes.size() + suffix.size());
    payload.insert(payload.end(), prefix.begin(), prefix.end());
    payload.insert(payload.end(), zipBytes.begin(), zipBytes.end());
    payload.insert(payload.end(), suffix.begin(), suffix.end());

    UrlParts parts;
    if (!ParseUrl(url, parts, result.error)) {
        AppendFailedUrl(result.error, url);
        return result;
    }

    HINTERNET session = WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        result.error = L"WinHttpOpen failed";
        AppendFailedUrl(result.error, url);
        return result;
    }
    WinHttpSetTimeouts(session, 30000, 30000, 120000, 120000);

    HINTERNET connect = WinHttpConnect(session, parts.host.c_str(), parts.port, 0);
    if (!connect) {
        result.error = L"WinHttpConnect failed";
        AppendFailedUrl(result.error, url);
        WinHttpCloseHandle(session);
        return result;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request =
        WinHttpOpenRequest(connect, L"POST", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        result.error = L"WinHttpOpenRequest failed";
        AppendFailedUrl(result.error, url);
        CloseHttpHandles(nullptr, connect, session);
        return result;
    }

    const std::wstring authHeader = L"Authorization: Bearer " + bearerToken;
    const std::wstring contentType = L"Content-Type: multipart/form-data; boundary=----MedicatFormBoundary9f2c1a7e";
    WinHttpAddRequestHeaders(request, authHeader.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(request, contentType.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);

    const BOOL sent = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, payload.data(),
                                         static_cast<DWORD>(payload.size()),
                                         static_cast<DWORD>(payload.size()), 0);
    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        result.error = L"HTTP multipart upload failed";
        AppendFailedUrl(result.error, url);
        CloseHttpHandles(request, connect, session);
        return result;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    result.statusCode = static_cast<int>(statusCode);

    std::vector<BYTE> responseBytes;
    std::wstring readError;
    ReadResponse(request, responseBytes, readError);
    CloseHttpHandles(request, connect, session);

    if (result.statusCode >= 200 && result.statusCode < 300 && !responseBytes.empty()) {
        const std::string responseJson(reinterpret_cast<const char*>(responseBytes.data()), responseBytes.size());
        result.keyword = ExtractJsonStringField(responseJson, "keyword");
        result.uploadId = ExtractJsonStringField(responseJson, "upload_id");
    } else if (!responseBytes.empty()) {
        const std::string responseJson(reinterpret_cast<const char*>(responseBytes.data()), responseBytes.size());
        const std::wstring apiError = ExtractJsonStringField(responseJson, "error");
        const std::wstring apiMessage = ExtractJsonStringField(responseJson, "message");
        if (!apiMessage.empty()) {
            result.error = apiMessage;
        } else if (!apiError.empty()) {
            result.error = apiError;
        } else if (!readError.empty()) {
            result.error = readError;
        }
    } else if (!readError.empty()) {
        result.error = readError;
    } else if (result.statusCode == 0) {
        result.error = L"HTTP upload failed";
    }

    if (!result.error.empty()) {
        AppendFailedUrl(result.error, url);
    }

    return result;
}

bool HttpGet(const std::wstring& url, std::wstring& body, std::wstring& error) {
    std::vector<BYTE> bytes;
    if (!HttpRequest(url, bytes, error)) {
        return false;
    }
    body = BytesToWide(bytes);
    return true;
}

bool HttpDownloadFile(const std::wstring& url, const std::wstring& outputPath, std::wstring& error) {
    return HttpDownloadFileWithProgress(url, outputPath, nullptr, error);
}

bool HttpDownloadFileWithProgress(const std::wstring& url, const std::wstring& outputPath,
                                  const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
                                  std::wstring& error) {
    if (!g_aria2cPath.empty() && FileExists(g_aria2cPath)) {
        const Aria2DownloadResult result =
            DownloadFileWithAria2(g_aria2cPath, url, outputPath, onProgress, error, false, 0);
        if (result == Aria2DownloadResult::Ok) {
            return true;
        }
        if (result == Aria2DownloadResult::Failed || FileExists(outputPath + L".aria2")) {
            return false;
        }
        error.clear();
    }
    return DownloadFileWithWinHttp(url, outputPath, onProgress, error);
}

bool TorrentDownloadFileWithProgress(const std::wstring& url, const std::wstring& outputPath,
                                     const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
                                     std::wstring& error) {
    if (g_aria2cPath.empty() || !FileExists(g_aria2cPath)) {
        error = L"aria2c is not available";
        return false;
    }

    std::wstring dir;
    std::wstring name;
    if (!SplitOutputPath(outputPath, dir, name)) {
        error = L"Invalid download path";
        return false;
    }

    const std::wstring torrentPath = JoinPath(GetMedicatTempDir(), L"MediCat_USB_v21.12.torrent");
    if (!DownloadFileWithWinHttp(url, torrentPath, nullptr, error)) {
        return false;
    }
    if (GetFileSizeBytes(torrentPath) < 32) {
        error = L"Torrent file was empty";
        AppendFailedUrl(error, url);
        return false;
    }

    int selectIndex = 0;
    if (!FindTorrentArchiveIndex(g_aria2cPath, torrentPath, name, selectIndex, error)) {
        return false;
    }

    const uint64_t existing = GetFileSizeBytes(outputPath);
    if (existing > 0 && existing < 1024ull * 1024 && !FileExists(outputPath + L".aria2")) {
        DeleteFileW(outputPath.c_str());
    }

    const Aria2DownloadResult result =
        DownloadFileWithAria2(g_aria2cPath, torrentPath, outputPath, onProgress, error, true, selectIndex);
    if (result != Aria2DownloadResult::Ok) {
        return false;
    }

    if (!FileExists(outputPath)) {
        const std::wstring nested = JoinPath(JoinPath(dir, L"MediCat USB v21.12"), name);
        if (FileExists(nested) && !MoveFileW(nested.c_str(), outputPath.c_str())) {
            error = L"Could not save archive file";
            return false;
        }
    }

    if (GetFileSizeBytes(outputPath) < 1024ull * 1024) {
        error = L"Torrent download did not retrieve the MediCat archive";
        return false;
    }
    return true;
}

bool TestInternetConnection(std::wstring& error) {
    std::wstring body;
    return HttpGet(L"https://api.github.com", body, error);
}

}  // namespace medicat
