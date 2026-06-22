#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace medicat {

enum class MediCatArchiveState { Missing, Incomplete, SizeOk, HashMismatch, Verified };

struct MediCatArchiveInfo {
    MediCatArchiveState state = MediCatArchiveState::Missing;
    std::wstring path;
    uint64_t sizeBytes = 0;
    std::string md5Hex;
};

std::wstring ResolveMediCatArchivePath(const std::wstring& root);
MediCatArchiveInfo InspectMediCatArchive(const std::wstring& path);
bool VerifyMediCatArchiveMd5(MediCatArchiveInfo& info, std::wstring& error);
bool IsMediCatArchiveReadyForInstall(
    const std::wstring& path, std::wstring& userMessage, std::wstring& userTitle,
    const std::function<void(const std::wstring&)>& onStatus = {},
    const std::function<void(const std::wstring&)>& onLog = {});

}  // namespace medicat
