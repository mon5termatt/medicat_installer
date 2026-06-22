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

}  // namespace

std::wstring ResolveMediCatArchivePath(const std::wstring& root) {
    const std::wstring besideExe = JoinPath(root, kMediCatArchiveFileName);
    if (FileExists(besideExe)) {
        return besideExe;
    }

    const std::wstring offline = ResolveOfflineArchivePath(kMediCatArchiveFileName);
    if (!offline.empty()) {
        return offline;
    }

    return besideExe;
}

MediCatArchiveInfo InspectMediCatArchive(const std::wstring& path) {
    MediCatArchiveInfo info{};
    info.path = path;
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

bool VerifyMediCatArchiveMd5(MediCatArchiveInfo& info, std::wstring& error) {
    error.clear();
    if (info.path.empty() || !FileExists(info.path)) {
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

    std::string md5Hex;
    if (!ComputeFileMd5(info.path, md5Hex, error)) {
        return false;
    }
    info.md5Hex = md5Hex;

    if (!Md5HexEquals(md5Hex, kMediCatArchiveMd5)) {
        info.state = MediCatArchiveState::HashMismatch;
        error = L"MD5 mismatch";
        return false;
    }

    info.state = MediCatArchiveState::Verified;
    return true;
}

bool IsMediCatArchiveReadyForInstall(
    const std::wstring& path, std::wstring& userMessage, std::wstring& userTitle,
    const std::function<void(const std::wstring&)>& onStatus,
    const std::function<void(const std::wstring&)>& onLog) {
    userMessage.clear();
    userTitle.clear();

    MediCatArchiveInfo info = InspectMediCatArchive(path);
    if (info.state == MediCatArchiveState::Missing) {
        userMessage = i18n::Tr(L"messages.archive_not_found", path);
        userTitle = i18n::Tr(L"titles.archive_not_found");
        return false;
    }

    if (info.state == MediCatArchiveState::Incomplete) {
        if (onLog) {
            onLog(i18n::Tr(L"log.archive_incomplete", std::to_wstring(info.sizeBytes),
                           std::to_wstring(kMediCatArchiveMinBytes)));
        }
        userMessage = i18n::Tr(L"messages.archive_incomplete", kMediCatArchiveFileName,
                               FormatBytes(info.sizeBytes), FormatBytes(kMediCatArchiveMinBytes));
        userTitle = i18n::Tr(L"titles.archive_incomplete");
        return false;
    }

    if (onStatus) {
        onStatus(i18n::Tr(L"status.verifying_archive"));
    }
    std::wstring hashError;
    if (!VerifyMediCatArchiveMd5(info, hashError)) {
        if (info.state == MediCatArchiveState::HashMismatch) {
            if (onLog) {
                onLog(i18n::Tr(L"log.archive_hash_mismatch", FormatMd5HexForDisplay(info.md5Hex),
                               Utf8ToWide(kMediCatArchiveMd5)));
            }
            userMessage = i18n::Tr(L"messages.archive_corrupt", kMediCatArchiveFileName, Utf8ToWide(kMediCatArchiveMd5));
            userTitle = i18n::Tr(L"titles.archive_corrupt");
            return false;
        }

        userMessage = i18n::Tr(L"messages.archive_corrupt", kMediCatArchiveFileName, Utf8ToWide(kMediCatArchiveMd5));
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
