#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace medicat {

void SetAria2cPath(const std::wstring& path);

bool HttpGet(const std::wstring& url, std::wstring& body, std::wstring& error);
int HttpPostJson(const std::wstring& url, const std::string& jsonBody, const std::wstring& bearerToken,
                 std::wstring& error);

struct HttpMultipartResult {
    int statusCode = 0;
    std::wstring keyword;
    std::wstring uploadId;
    std::wstring error;
};

HttpMultipartResult HttpPostMultipartUpload(const std::wstring& url, const std::wstring& bearerToken,
                                            const std::wstring& sessionId, const std::string& manifestJson,
                                            const std::wstring& zipPath);
bool HttpDownloadFile(const std::wstring& url, const std::wstring& outputPath, std::wstring& error);
bool HttpDownloadFileWithProgress(const std::wstring& url, const std::wstring& outputPath,
                                  const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
                                  std::wstring& error);
bool TorrentDownloadFileWithProgress(const std::wstring& url, const std::wstring& outputPath,
                                     const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
                                     std::wstring& error);
bool TestInternetConnection(std::wstring& error);

}  // namespace medicat
