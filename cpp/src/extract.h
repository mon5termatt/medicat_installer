#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace medicat {

struct ExtractProgress {
    int percent = 0;
    std::wstring file;
    uint64_t bytesWritten = 0;
    uint64_t totalBytes = 0;
};

struct ExtractResult {
    bool success = false;
    int exitCode = -1;
    std::wstring error;
};

using ProgressCallback = std::function<void(const ExtractProgress&)>;

ExtractResult Extract7zArchive(
    const std::wstring& sevenZipExe,
    const std::wstring& archivePath,
    const std::wstring& destinationRoot,
    uint64_t totalUncompressedBytes,
    uint64_t initialFreeBytes,
    ProgressCallback onProgress);

}  // namespace medicat
