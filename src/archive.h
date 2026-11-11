#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "downloads.h"
#include "verify.h"

namespace medicat {

enum class MediCatArchiveState { Missing, Incomplete, SizeOk, HashMismatch, Verified };
enum class MediCatArchiveKind { Solid7z, SplitZip };

struct MediCatArchiveInfo {
    MediCatArchiveState state = MediCatArchiveState::Missing;
    MediCatArchiveKind kind = MediCatArchiveKind::Solid7z;
    std::wstring path;  // extract path: .7z or .zip.001
    uint64_t sizeBytes = 0;
    uint64_t expectedMinBytes = 0;
    std::string md5Hex;
    std::wstring displayName;     // for user messages
    std::string expectedMd5Hex;   // for corrupt messages (solid or failing part)
    std::wstring failingPartName; // set on split hash mismatch
};

// Returns path to pass to 7za (.7z or .zip.001). Prefers solid .7z when present.
// Split zip volumes apply to the default (21.12) release only.
std::wstring ResolveMediCatArchivePath(const std::wstring& root,
                                       const MediCatReleaseInfo& release = DefaultMediCatRelease());
// If override points at .zip.002-.006, normalize to .001 in the same folder.
std::wstring NormalizeMediCatArchivePath(const std::wstring& path);
MediCatArchiveInfo InspectMediCatArchive(const std::wstring& path,
                                         const MediCatReleaseInfo& release = DefaultMediCatRelease());
bool VerifyMediCatArchiveMd5(MediCatArchiveInfo& info, const MediCatReleaseInfo& release, std::wstring& error,
                             const FileHashProgressFn& onProgress = {});
bool IsMediCatArchiveReadyForInstall(
    const std::wstring& path, const MediCatReleaseInfo& release, std::wstring& userMessage,
    std::wstring& userTitle, const std::function<void(const std::wstring&)>& onStatus = {},
    const std::function<void(const std::wstring&)>& onLog = {},
    const FileHashProgressFn& onProgress = {});

}  // namespace medicat
