#pragma once

#include <windows.h>
#include <shellapi.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace medicat {

struct MediCatReleaseInfo {
    const wchar_t* version = nullptr;          // e.g. L"21.12"
    const wchar_t* archiveFileName = nullptr;  // e.g. L"MediCat.USB.v21.12.7z"
    uint64_t fullBytes = 0;                    // 0 = unknown size (skip min-size gate)
    const char* md5 = nullptr;                 // empty/null = skip MD5 gate
    const wchar_t* mirror1Url = nullptr;
    const wchar_t* mirror2Url = nullptr;
    const wchar_t* torrentUrl = nullptr;
    const wchar_t* magnetUrl = nullptr;
    const wchar_t* googleDriveUrl = nullptr;
    const wchar_t* megaUrl = nullptr;
};

// Known MediCat USB releases selectable in Advanced options.
// All current public download links are 21.12; 26.12 slots stay empty until new links land.
inline constexpr MediCatReleaseInfo kMediCatReleases[] = {
    {
        L"21.12",
        L"MediCat.USB.v21.12.7z",
        22994783619ULL,
        "db50f96a5c7b5ec6dc9ed77ea29fffb0",
        L"https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z",
        L"https://files.dog/OD%20Rips/MediCat/v21.12/MediCat.USB.v21.12.7z",
        L"https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_v21.12.torrent",
        L"magnet:?xt=urn:btih:1D714BDF37890669E98933B724B55D47E7F2D01B",
        L"https://drive.google.com/drive/folders/0B80MkZEQZejCSDZsY2NKbVJNWjg?resourcekey=0-wMGHCpg8SwR13oOYDfjPuA",
        L"https://mega.nz/folder/jg1DWbaK#Qo6XsYzjx-HyIpxj8xQTiQ",
    },
    {
        L"26.12",
        L"MediCat.USB.v26.12.7z",
        0,
        "",
        L"",
        L"",
        L"",
        L"",
        L"",
        L"",
    },
};

inline constexpr size_t kMediCatReleaseCount = sizeof(kMediCatReleases) / sizeof(kMediCatReleases[0]);
inline constexpr size_t kDefaultMediCatReleaseIndex = 0;

// Compatibility aliases for the default (21.12) release.
constexpr wchar_t kMediCatArchiveFileName[] = L"MediCat.USB.v21.12.7z";
constexpr uint64_t kMediCatArchiveFullBytes = 22994783619ULL;
constexpr uint64_t kMediCatArchiveMinBytes = kMediCatArchiveFullBytes * 95 / 100;
constexpr char kMediCatArchiveMd5[] = "db50f96a5c7b5ec6dc9ed77ea29fffb0";

// Google Drive / Mega multi-volume zip (hasher/drivefiles.md5). Extract via .001 with 7za.
struct MediCatSplitPart {
    const wchar_t* fileName;
    uint64_t fullBytes;
    const char* md5;   // lowercase hex
    const char* sha1;  // reference only; installer verifies MD5
};

constexpr MediCatSplitPart kMediCatSplitParts[] = {
    {L"MediCat.USB.v21.12.zip.001", 4290772992ULL, "277793dcf0e31736f0790162a89d07c9",
     "ef13910a43c1ae9d04a86b2853f11e44cf7ae193"},
    {L"MediCat.USB.v21.12.zip.002", 4290772992ULL, "a4700261f32d4df5092c5dd5ea6aaa2d",
     "6106453ea19646a889864328758886c3726c96f2"},
    {L"MediCat.USB.v21.12.zip.003", 4290772992ULL, "6b523273c5c7ed1ddc5920dec95b8509",
     "69a5c48bdba8fe7ae33b9d251813230f2868d731"},
    {L"MediCat.USB.v21.12.zip.004", 4290772992ULL, "35fac6ff4902d62e6e5fd2dab1050a3f",
     "ba7d0aa2f55bc9c96bad3d47f762d1dd324fc021"},
    {L"MediCat.USB.v21.12.zip.005", 4290772992ULL, "7f416a7d9ff0051ae75bbf44a411b8e4",
     "543803780e5fc6a0f3de05791b1b48d371d23215"},
    {L"MediCat.USB.v21.12.zip.006", 2917026620ULL, "32d84a280af91ae408f55a7722ee6818",
     "de48d627bc9baaff7646357e87f4a6cd7f6bb450"},
};
constexpr size_t kMediCatSplitPartCount = sizeof(kMediCatSplitParts) / sizeof(kMediCatSplitParts[0]);
constexpr wchar_t kMediCatSplitFirstFileName[] = L"MediCat.USB.v21.12.zip.001";

constexpr wchar_t kDownloadMirror1Url[] =
    L"https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z";

// Optional update pack beside the exe / offline (e.g. MediCat.USB.v21.12.update.7z).
constexpr wchar_t kMediCatUpdatePackGlob[] = L"MediCat.USB.*update.7z";

constexpr wchar_t kDownloadAllUrl[] = L"http://medicatusb.com/#downloads";
constexpr wchar_t kManualInstallDocUrl[] = L"https://medicatusb.com/docs/medicat/installation/manual-install/";
constexpr wchar_t kBetaFeedbackUrl[] = L"https://github.com/mon5termatt/medicat_installer/issues";
constexpr wchar_t kDiscordSupportUrl[] = L"https://url.medicatusb.com/discord";
constexpr wchar_t kSevenZipProjectUrl[] = L"https://www.7-zip.org/";
constexpr wchar_t kAria2ProjectUrl[] = L"https://aria2.github.io/";
constexpr wchar_t kVentoyProjectUrl[] = L"https://www.ventoy.net/en/index.html";

inline bool HasReleaseUrl(const wchar_t* url) {
    return url != nullptr && url[0] != L'\0';
}

inline const MediCatReleaseInfo& DefaultMediCatRelease() {
    return kMediCatReleases[kDefaultMediCatReleaseIndex];
}

inline const MediCatReleaseInfo* FindMediCatRelease(const std::wstring& version) {
    for (size_t i = 0; i < kMediCatReleaseCount; ++i) {
        if (version == kMediCatReleases[i].version) {
            return &kMediCatReleases[i];
        }
    }
    return nullptr;
}

inline const MediCatReleaseInfo& ResolveMediCatRelease(const std::wstring& version) {
    if (const MediCatReleaseInfo* found = FindMediCatRelease(version)) {
        return *found;
    }
    return DefaultMediCatRelease();
}

inline uint64_t MediCatReleaseMinBytes(const MediCatReleaseInfo& release) {
    if (release.fullBytes == 0) {
        return 1;
    }
    return release.fullBytes * 95 / 100;
}

inline bool MediCatReleaseHasMd5(const MediCatReleaseInfo& release) {
    return release.md5 != nullptr && release.md5[0] != '\0';
}

inline bool MediCatReleaseHasMirrors(const MediCatReleaseInfo& release) {
    return HasReleaseUrl(release.mirror1Url) || HasReleaseUrl(release.mirror2Url);
}

inline bool MediCatReleaseHasAltDownloads(const MediCatReleaseInfo& release) {
    return HasReleaseUrl(release.torrentUrl) || HasReleaseUrl(release.magnetUrl) ||
           HasReleaseUrl(release.googleDriveUrl) || HasReleaseUrl(release.megaUrl);
}

inline bool OpenBrowserUrl(const wchar_t* url) {
    if (!HasReleaseUrl(url)) {
        return false;
    }
    const HINSTANCE result =
        ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

}  // namespace medicat
