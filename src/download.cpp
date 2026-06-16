#include "download.hpp"

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
    UrlParts parts;
    if (!ParseUrl(url, parts, error)) {
        return false;
    }

    HINTERNET session =
        WinHttpOpen(L"MedicatInstaller/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        error = L"WinHttpOpen failed";
        return false;
    }

    WinHttpSetTimeouts(session, 30000, 30000, 120000, 120000);

    HINTERNET connect = WinHttpConnect(session, parts.host.c_str(), parts.port, 0);
    if (!connect) {
        WinHttpCloseHandle(session);
        error = L"WinHttpConnect failed";
        return false;
    }

    const DWORD flags = parts.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", parts.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        error = L"WinHttpOpenRequest failed";
        return false;
    }

    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));

    const wchar_t* headers = L"User-Agent: MedicatInstaller/1.0\r\nAccept: application/json,*/*";
    if (!WinHttpSendRequest(request, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        error = L"HTTP request failed";
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                             WINHTTP_NO_HEADER_INDEX) ||
        statusCode < 200 || statusCode >= 300) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        error = L"HTTP request returned status " + std::to_wstring(statusCode);
        return false;
    }

    const bool ok = ReadResponse(request, body, error);
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
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

}  // namespace

bool HttpGet(const std::wstring& url, std::wstring& body, std::wstring& error) {
    std::vector<BYTE> bytes;
    if (!HttpRequest(url, bytes, error)) {
        return false;
    }
    body = BytesToWide(bytes);
    return true;
}

bool HttpDownloadFile(const std::wstring& url, const std::wstring& outputPath, std::wstring& error) {
    std::vector<BYTE> bytes;
    if (!HttpRequest(url, bytes, error)) {
        return false;
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        error = L"Could not create output file";
        return false;
    }
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out.good();
}

bool TestInternetConnection(std::wstring& error) {
    std::wstring body;
    return HttpGet(L"https://api.github.com", body, error);
}

}  // namespace medicat
