#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace medicat {

struct InstallerUpdateInfo {
    bool updateAvailable = false;
    int remoteBuild = 0;
    std::wstring version;
    std::wstring releaseTag;
    std::wstring releaseUrl;
    std::wstring downloadUrl;
};

struct UpdateCheckResult {
    bool success = false;
    std::wstring error;
    InstallerUpdateInfo info;
};

UpdateCheckResult CheckForInstallerUpdate();

std::wstring GetInstallerAssetFileName();
bool DownloadAndRelaunchInstallerUpdate(const InstallerUpdateInfo& info,
                                        const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
                                        const std::function<void(const std::wstring&)>& onLog, std::wstring& error);

}  // namespace medicat
