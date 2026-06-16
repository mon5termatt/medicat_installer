#pragma once

#include <shellapi.h>
#include <windows.h>

namespace medicat {

// From mon5termatt/medicat-website (altDownloads.json + downloads.astro).
constexpr wchar_t kMediCatArchiveFileName[] = L"MediCat.USB.v21.12.7z";

constexpr wchar_t kDownloadMirror1Name[] = L"files.dog";
constexpr wchar_t kDownloadMirror1Url[] =
    L"https://files.dog/OD%20Rips/MediCat/v21.12/MediCat.USB.v21.12.7z";

constexpr wchar_t kDownloadMirror2Name[] = L"files.medicatusb.com";
constexpr wchar_t kDownloadMirror2Url[] =
    L"https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z";

constexpr wchar_t kDownloadTorrentUrl[] =
    L"https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_v21.12.torrent";
constexpr wchar_t kDownloadMagnetUrl[] =
    L"magnet:?xt=urn:btih:1D714BDF37890669E98933B724B55D47E7F2D01B";
constexpr wchar_t kDownloadGoogleDriveUrl[] =
    L"https://drive.google.com/drive/folders/0B80MkZEQZejCSDZsY2NKbVJNWjg?resourcekey=0-wMGHCpg8SwR13oOYDfjPuA";
constexpr wchar_t kDownloadMegaUrl[] = L"https://mega.nz/folder/jg1DWbaK#Qo6XsYzjx-HyIpxj8xQTiQ";
constexpr wchar_t kDownloadAllUrl[] = L"http://medicatusb.com/#downloads";

struct AlternativeDownloadOption {
    const wchar_t* labelKey;
    const wchar_t* url;
};

constexpr AlternativeDownloadOption kAlternativeDownloads[] = {
    {L"ui.download_torrent", kDownloadTorrentUrl},
    {L"ui.download_magnet", kDownloadMagnetUrl},
    {L"ui.download_google_drive", kDownloadGoogleDriveUrl},
    {L"ui.download_mega", kDownloadMegaUrl},
    {L"ui.download_all", kDownloadAllUrl},
};

inline bool OpenBrowserUrl(const wchar_t* url) {
    if (!url || !*url) {
        return false;
    }
    const HINSTANCE result =
        ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

}  // namespace medicat
