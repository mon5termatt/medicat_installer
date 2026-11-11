#pragma once

#include <functional>
#include <string>
#include <vector>

#include "extract.h"
#include "verify.h"

namespace medicat {

struct UpdatePackInfo {
    std::wstring archivePath;
    std::wstring name;
    std::wstring version;
    std::wstring base;
    // Always "." : USB-relative payload paths live at the archive root.
    std::wstring contentRoot;
    std::wstring manifest;
    std::string archiveMd5;
    // From info.json diff.bootablesOnly: pack must not touch PortableApps/ or Programs/.
    bool bootablesOnly = false;
    std::vector<std::wstring> remove;
    std::vector<std::wstring> add;
};

struct UpdatePackResolveResult {
    bool found = false;
    std::wstring path;
    std::vector<std::wstring> candidates;
};

struct UpdatePackApplyResult {
    bool success = false;
    bool skipped = false;
    std::wstring error;
    UpdatePackInfo info;
    std::wstring extractedManifestPath;
    std::wstring workDir;
};

// Discover MediCat.USB.*update.7z beside exe, then offline/. Newest LastWriteTime wins.
UpdatePackResolveResult ResolveUpdatePackPath(const std::wstring& installerRoot,
                                             const std::wstring& browseOverride = L"");

bool MatchesUpdatePackGlob(const std::wstring& fileName);

bool ParseUpdatePackInfoJson(const std::string& jsonUtf8, UpdatePackInfo& info, std::wstring& error);

// Extract info.json (+ optional full validation of manifest fingerprint / members).
bool LoadUpdatePackInfo(const std::wstring& sevenZipExe, const std::wstring& archivePath,
                        const std::wstring& workDir, UpdatePackInfo& info, std::wstring& error,
                        const std::wstring& extractLogPath = L"");

bool ValidateUpdatePackInfo(const UpdatePackInfo& info, const std::wstring& workDir,
                            std::wstring& error);

// Delete remove[] paths on the drive, extract add[] straight onto the drive (temp only for
// info.json / manifest), leave manifest in workDir.
UpdatePackApplyResult ApplyUpdatePack(
    const std::wstring& sevenZipExe, const std::wstring& archivePath, const std::wstring& driveRoot,
    const std::wstring& workDir, ProgressCallback onProgress,
    const std::function<void(const std::wstring&)>& onLog = {},
    const std::wstring& extractLogPath = L"");

}  // namespace medicat
