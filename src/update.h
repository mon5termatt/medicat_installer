#pragma once

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

}  // namespace medicat
