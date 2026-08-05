#include "archive.h"

#include <cwctype>

#include <windows.h>

#include "downloads.h"
#include "i18n.h"
#include "offline.h"
#include "util.h"
#include "verify.h"

#include <algorithm>
#include <cctype>

namespace medicat {

namespace {

bool Md5HexEquals(const std::string& actual, const char* expected) {
    if (actual.size() != 32 || !expected) {
        return false;
    }
    for (size_t i = 0; i < 32; ++i) {
        if (std::tolower(static_cast<unsigned char>(actual[i])) !=
            std::tolower(static_cast<unsigned char>(expected[i]))) {
            return false;
        }
    }
    return true;
}

std::wstring FormatMd5HexForDisplay(const std::string& hex) {
    std::wstring wide = Utf8ToWide(hex);
    std::transform(wide.begin(), wide.end(), wide.begin(),
                   [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
    return wide;
}

std::wstring ParentDirectory(const std::wstring& path) {
    const auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    if (pos == 0) {
        return path.substr(0, 1);
    }
    return path.substr(0, pos);
}

std::wstring FileNameOnly(const std::wstring& path) {
    const auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

bool EndsWithIgnoreCase(const std::wstring& value, const std::wstring& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    for (size_t i = 0; i < suffix.size(); ++i) {
        const wchar_t a = towlower(value[value.size() - suffix.size() + i]);
        const wchar_t b = towlower(suffix[i]);
        if (a != b) {
            return false;
        }
    }
    return true;
}

bool EqualsIgnoreCase(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) {
            return false;
        }
    }
    return true;
}

int SplitPartIndexFromFileName(const std::wstring& fileName) {
    for (size_t i = 0; i < kMediCatSplitPartCount; ++i) {
        if (EqualsIgnoreCase(fileName, kMediCatSplitParts[i].fileName)) {
            return static_cast<int>(i);
        }
    }
    // Accept *.zip.001 … *.zip.006 (same folder volume set for 7za).
    static const wchar_t* kSuffixes[] = {L".zip.001", L".zip.002", L".zip.003",
                                         L".zip.004", L".zip.005", L".zip.006"};
    for (size_t i = 0; i < kMediCatSplitPartCount; ++i) {
        if (EndsWithIgnoreCase(fileName, kSuffixes[i])) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool IsSplitZipPath(const std::wstring& path) {
    return SplitPartIndexFromFileName(FileNameOnly(path)) >= 0;
}

uint64_t SplitPartMinBytes(const MediCatSplitPart& part) {
    return part.fullBytes * 95 / 100;
}

uint64_t TotalSplitFullBytes() {
    uint64_t total = 0;
    for (size_t i = 0; i < kMediCatSplitPartCount; ++i) {
        total += kMediCatSplitParts[i].fullBytes;
    }
    return total;
}

uint64_t TotalSplitMinBytes() {
    return TotalSplitFullBytes() * 95 / 100;
}

bool AnySplitPartExistsInDirectory(const std::wstring& directory) {
    for (size_t i = 0; i < kMediCatSplitPartCount; ++i) {
        if (FileExists(JoinPath(directory, kMediCatSplitParts[i].fileName))) {
            return true;
        }
    }
    return false;
}

MediCatArchiveInfo InspectSolidArchive(const std::wstring& path) {
    MediCatArchiveInfo info{};
    info.kind = MediCatArchiveKind::Solid7z;
    info.path = path;
    info.displayName = kMediCatArchiveFileName;
    info.expectedMd5Hex = kMediCatArchiveMd5;
    info.expectedMinBytes = kMediCatArchiveMinBytes;

    if (path.empty() || !FileExists(path)) {
        return info;
    }

    info.sizeBytes = GetFileSizeBytes(path);
    if (info.sizeBytes < kMediCatArchiveMinBytes) {
        info.state = MediCatArchiveState::Incomplete;
        return info;
    }

    info.state = MediCatArchiveState::SizeOk;
    return info;
}

MediCatArchiveInfo InspectSplitArchive(const std::wstring& firstVolumePath) {
    MediCatArchiveInfo info{};
    info.kind = MediCatArchiveKind::SplitZip;
    info.path = firstVolumePath;
    info.displayName = kMediCatSplitFirstFileName;
    info.expectedMinBytes = TotalSplitMinBytes();

    const std::wstring directory = ParentDirectory(firstVolumePath);
    bool anyExists = false;
    bool allSizeOk = true;
    uint64_t totalSize = 0;

    for (size_t i = 0; i < kMediCatSplitPartCount; ++i) {
        const auto& part = kMediCatSplitParts[i];
        const std::wstring partPath = JoinPath(directory, part.fileName);
        if (!FileExists(partPath)) {
            allSizeOk = false;
            continue;
        }
        anyExists = true;
        const uint64_t size = GetFileSizeBytes(partPath);
        totalSize += size;
        if (size < SplitPartMinBytes(part)) {
            allSizeOk = false;
        }
    }

    info.sizeBytes = totalSize;
    if (!anyExists) {
        info.state = MediCatArchiveState::Missing;
        return info;
    }
    if (!allSizeOk) {
        info.state = MediCatArchiveState::Incomplete;
        return info;
    }

    info.state = MediCatArchiveState::SizeOk;
    return info;
}

bool VerifySolidMd5(MediCatArchiveInfo& info, std::wstring& error, const FileHashProgressFn& onProgress) {
    std::string md5Hex;
    if (!ComputeFileMd5(info.path, md5Hex, error, nullptr, onProgress)) {
        return false;
    }
    info.md5Hex = md5Hex;

    if (!Md5HexEquals(md5Hex, kMediCatArchiveMd5)) {
        info.state = MediCatArchiveState::HashMismatch;
        info.expectedMd5Hex = kMediCatArchiveMd5;
        error = L"MD5 mismatch";
        return false;
    }

    info.state = MediCatArchiveState::Verified;
    return true;
}

bool VerifySplitMd5(MediCatArchiveInfo& info, std::wstring& error, const FileHashProgressFn& onProgress) {
    const std::wstring directory = ParentDirectory(info.path);
    const uint64_t totalBytes = TotalSplitFullBytes();
    uint64_t bytesBeforePart = 0;

    for (size_t i = 0; i < kMediCatSplitPartCount; ++i) {
        const auto& part = kMediCatSplitParts[i];
        const std::wstring partPath = JoinPath(directory, part.fileName);
        if (!FileExists(partPath)) {
            info.state = MediCatArchiveState::Incomplete;
            info.failingPartName = part.fileName;
            error = L"Split archive part missing: " + std::wstring(part.fileName);
            return false;
        }

        FileHashProgressFn partProgress;
        if (onProgress) {
            partProgress = [&](uint64_t bytesRead, uint64_t /*partTotal*/) {
                onProgress(bytesBeforePart + bytesRead, totalBytes);
            };
        }

        std::string md5Hex;
        if (!ComputeFileMd5(partPath, md5Hex, error, nullptr, partProgress)) {
            info.failingPartName = part.fileName;
            return false;
        }

        if (!Md5HexEquals(md5Hex, part.md5)) {
            info.state = MediCatArchiveState::HashMismatch;
            info.md5Hex = md5Hex;
            info.expectedMd5Hex = part.md5;
            info.failingPartName = part.fileName;
            info.displayName = part.fileName;
            error = L"MD5 mismatch";
            return false;
        }

        bytesBeforePart += GetFileSizeBytes(partPath);
        info.md5Hex = md5Hex;
    }

    info.state = MediCatArchiveState::Verified;
    info.displayName = kMediCatSplitFirstFileName;
    return true;
}

}  // namespace

std::wstring NormalizeMediCatArchivePath(const std::wstring& path) {
    if (path.empty()) {
        return path;
    }

    const int index = SplitPartIndexFromFileName(FileNameOnly(path));
    if (index < 0) {
        return path;
    }
    if (index == 0) {
        return path;
    }

    return JoinPath(ParentDirectory(path), kMediCatSplitFirstFileName);
}

std::wstring ResolveMediCatArchivePath(const std::wstring& root) {
    const std::wstring sevenZBeside = JoinPath(root, kMediCatArchiveFileName);
    if (FileExists(sevenZBeside)) {
        return sevenZBeside;
    }

    const std::wstring sevenZOffline = ResolveOfflineArchivePath(kMediCatArchiveFileName);
    if (!sevenZOffline.empty()) {
        return sevenZOffline;
    }

    const std::wstring splitBeside = JoinPath(root, kMediCatSplitFirstFileName);
    if (FileExists(splitBeside) || AnySplitPartExistsInDirectory(root)) {
        return splitBeside;
    }

    const std::wstring offlineDir = GetOfflineDirectory();
    const std::wstring splitOfflineFirst = ResolveOfflineArchivePath(kMediCatSplitFirstFileName);
    if (!splitOfflineFirst.empty()) {
        return splitOfflineFirst;
    }
    if (!offlineDir.empty() && AnySplitPartExistsInDirectory(offlineDir)) {
        return JoinPath(offlineDir, kMediCatSplitFirstFileName);
    }

    return sevenZBeside;
}

MediCatArchiveInfo InspectMediCatArchive(const std::wstring& path) {
    const std::wstring normalized = NormalizeMediCatArchivePath(path);
    if (normalized.empty()) {
        return InspectSolidArchive(path);
    }

    if (IsSplitZipPath(normalized)) {
        return InspectSplitArchive(normalized);
    }

    // Resolve may still point at the default .7z name while only Drive parts exist.
    if (EqualsIgnoreCase(FileNameOnly(normalized), kMediCatArchiveFileName) && !FileExists(normalized)) {
        const std::wstring directory = ParentDirectory(normalized);
        if (AnySplitPartExistsInDirectory(directory)) {
            return InspectSplitArchive(JoinPath(directory, kMediCatSplitFirstFileName));
        }
    }

    return InspectSolidArchive(normalized);
}

bool VerifyMediCatArchiveMd5(MediCatArchiveInfo& info, std::wstring& error, const FileHashProgressFn& onProgress) {
    error.clear();
    if (info.path.empty()) {
        info.state = MediCatArchiveState::Missing;
        error = L"Archive file not found";
        return false;
    }

    if (info.kind == MediCatArchiveKind::SplitZip) {
        if (info.state == MediCatArchiveState::Missing) {
            error = L"Archive file not found";
            return false;
        }
        if (info.state == MediCatArchiveState::Incomplete || info.sizeBytes < info.expectedMinBytes) {
            if (info.state != MediCatArchiveState::Incomplete) {
                info.state = MediCatArchiveState::Incomplete;
            }
            error = L"Archive file is too small";
            return false;
        }
        return VerifySplitMd5(info, error, onProgress);
    }

    if (!FileExists(info.path)) {
        info.state = MediCatArchiveState::Missing;
        error = L"Archive file not found";
        return false;
    }

    if (info.sizeBytes == 0) {
        info.sizeBytes = GetFileSizeBytes(info.path);
    }
    if (info.sizeBytes < kMediCatArchiveMinBytes) {
        info.state = MediCatArchiveState::Incomplete;
        error = L"Archive file is too small";
        return false;
    }

    return VerifySolidMd5(info, error, onProgress);
}

bool IsMediCatArchiveReadyForInstall(
    const std::wstring& path, std::wstring& userMessage, std::wstring& userTitle,
    const std::function<void(const std::wstring&)>& onStatus,
    const std::function<void(const std::wstring&)>& onLog,
    const FileHashProgressFn& onProgress) {
    userMessage.clear();
    userTitle.clear();

    const std::wstring normalized = NormalizeMediCatArchivePath(path);
    MediCatArchiveInfo info = InspectMediCatArchive(normalized);
    if (info.state == MediCatArchiveState::Missing) {
        userMessage = i18n::Tr(L"messages.archive_not_found", normalized.empty() ? path : normalized);
        userTitle = i18n::Tr(L"titles.archive_not_found");
        return false;
    }

    if (info.state == MediCatArchiveState::Incomplete) {
        const uint64_t minBytes =
            info.expectedMinBytes > 0 ? info.expectedMinBytes
                                      : (info.kind == MediCatArchiveKind::SplitZip ? TotalSplitMinBytes()
                                                                                   : kMediCatArchiveMinBytes);
        if (onLog) {
            onLog(i18n::Tr(L"log.archive_incomplete", std::to_wstring(info.sizeBytes),
                           std::to_wstring(minBytes)));
        }
        userMessage = i18n::Tr(L"messages.archive_incomplete", info.displayName.c_str(),
                               FormatBytes(info.sizeBytes), FormatBytes(minBytes));
        userTitle = i18n::Tr(L"titles.archive_incomplete");
        return false;
    }

    if (onStatus) {
        onStatus(i18n::Tr(L"status.verifying_archive"));
    }
    std::wstring hashError;
    if (!VerifyMediCatArchiveMd5(info, hashError, onProgress)) {
        const std::wstring expectedMd5 = Utf8ToWide(
            info.expectedMd5Hex.empty()
                ? (info.kind == MediCatArchiveKind::Solid7z ? kMediCatArchiveMd5 : "")
                : info.expectedMd5Hex);
        const std::wstring corruptName =
            !info.failingPartName.empty() ? info.failingPartName : info.displayName;

        if (info.state == MediCatArchiveState::HashMismatch) {
            if (onLog) {
                onLog(i18n::Tr(L"log.archive_hash_mismatch", FormatMd5HexForDisplay(info.md5Hex), expectedMd5));
            }
            userMessage = i18n::Tr(L"messages.archive_corrupt", corruptName.c_str(), expectedMd5);
            userTitle = i18n::Tr(L"titles.archive_corrupt");
            return false;
        }

        userMessage = i18n::Tr(L"messages.archive_corrupt", corruptName.c_str(), expectedMd5);
        if (!hashError.empty()) {
            userMessage += L"\n\n" + hashError;
        }
        userTitle = i18n::Tr(L"titles.archive_corrupt");
        return false;
    }

    if (onLog) {
        onLog(i18n::Tr(L"log.archive_verified", FormatBytes(info.sizeBytes)));
    }
    return true;
}

}  // namespace medicat
