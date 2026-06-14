#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace medicat {

struct VerifyProgress {
    size_t current = 0;
    size_t total = 0;
    std::wstring file;
};

struct VerifyFileLogEntry {
    size_t current = 0;
    size_t total = 0;
    std::wstring line;
};

struct VerifyResult {
    bool success = false;
    size_t totalFiles = 0;
    size_t skippedFiles = 0;
    size_t verifiedFiles = 0;
    size_t failedFiles = 0;
    size_t missingFiles = 0;
    size_t hashMismatchFiles = 0;
    std::vector<std::wstring> failures;
    std::wstring error;
    std::wstring failedListPath;
    std::wstring checkLogPath;
};

struct VerifyOptions {
    std::wstring driveRoot;
    std::wstring installerRoot;
    std::wstring tempDir;
    std::wstring manifestPath;
    std::wstring failedListPath;
    std::wstring checkLogPath;
    std::function<void(const VerifyProgress&)> onProgress;
    std::function<void(const VerifyFileLogEntry&)> onFileLog;
    std::function<void(const std::wstring&)> onLog;
    size_t threadCount = 0;
};

bool EnsureMedicatMd5Manifest(const std::wstring& installerRoot, const std::wstring& tempDir,
                              std::wstring& manifestPath, std::wstring& error);
VerifyResult VerifyMedicatFiles(const VerifyOptions& options);

}  // namespace medicat
