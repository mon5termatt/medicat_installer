#include "download.h"

#include "util.h"

#include <windows.h>
#include <winhttp.h>

#include <fstream>
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

bool OpenHttpGetRequest(const std::wstring& url, HINTERNET& outRequest, HINTERNET& outConnect,
                        HINTERNET& outSession, uint64_t& contentLength, std::wstring& error) {
    contentLength = 0;
    outRequest = nullptr;
    outConnect = nullptr;
    outSession = nullptr;

    UrlParts parts;
    if (!ParseUrl(url, parts, error)) {
        return false;
    }

    outSession = WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS, 0);
    if (!outSession) {
        error = L"WinHttpOpen failed";
        return false;
    }

    WinHttpSetTimeouts(outSession, 30000, 30000, 300000, 1800000);

    outConnect = WinHttpConnect(outSession, parts.host.c_str(), parts.port, 0);
    if (!outConnect) {
        error = L"WinHttpConnect failed";
        return false;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    outRequest = WinHttpOpenRequest(outConnect, L"GET", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                    WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!outRequest) {
        error = L"WinHttpOpenRequest failed";
        return false;
    }

    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(outRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));

    const wchar_t* headers = L"User-Agent: MedicatInstaller/1.0\r\nAccept: */*";
    if (!WinHttpSendRequest(outRequest, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(outRequest, nullptr)) {
        error = L"HTTP request failed";
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(outRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
        statusCode < 200 || statusCode >= 300) {
        error = L"HTTP request returned status " + std::to_wstring(statusCode);
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
        return false;
    }

    outSession = WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS, 0);
    if (!outSession) {
        error = L"WinHttpOpen failed";
        return false;
    }

    WinHttpSetTimeouts(outSession, 30000, 30000, 300000, 1800000);

    outConnect = WinHttpConnect(outSession, parts.host.c_str(), parts.port, 0);
    if (!outConnect) {
        error = L"WinHttpConnect failed";
        return false;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    outRequest = WinHttpOpenRequest(outConnect, L"GET", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                    WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!outRequest) {
        error = L"WinHttpOpenRequest failed";
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
        return false;
    }

    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(outRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
        error = L"HTTP request failed";
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
        return false;
    }

    if (statusCode == 416) {
        uint64_t rangeEnd = 0;
        ReadContentRangeHeader(outRequest, rangeStart, rangeEnd, totalFileSize);
        return true;
    }

    if (statusCode < 200 || statusCode >= 300) {
        error = L"HTTP request returned status " + std::to_wstring(statusCode);
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
        return 0;
    }

    HINTERNET session = WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        error = L"WinHttpOpen failed";
        return 0;
    }
    WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);

    HINTERNET connect = WinHttpConnect(session, parts.host.c_str(), parts.port, 0);
    if (!connect) {
        error = L"WinHttpConnect failed";
        WinHttpCloseHandle(session);
        return 0;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request =
        WinHttpOpenRequest(connect, L"POST", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        error = L"WinHttpOpenRequest failed";
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
    return static_cast<int>(statusCode);
}

}  // namespace

int HttpPostJson(const std::wstring& url, const std::string& jsonBody, const std::wstring& bearerToken,
                 std::wstring& error) {
    return HttpPostJsonInternal(url, jsonBody, bearerToken, error);
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
        return false;
    }

    if (statusCode != 200 && statusCode != 206) {
        CloseHttpHandles(request, connect, session);
        error = L"HTTP request returned status " + std::to_wstring(statusCode);
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
            return false;
        }

        out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(read));
        if (!out.good()) {
            out.close();
            CloseHttpHandles(request, connect, session);
            error = L"Failed to write output file";
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
        return false;
    }
    if (totalFileSize > 0 && finalSize < totalFileSize) {
        error = L"Download incomplete";
        return false;
    }

    return true;
}

bool TestInternetConnection(std::wstring& error) {
    std::wstring body;
    return HttpGet(L"https://api.github.com", body, error);
}

}  // namespace medicat
