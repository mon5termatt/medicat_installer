#include "update.h"

#include "download.h"
#include "util.h"

#include <windows.h>

#include <algorithm>
#include <cwctype>

#ifndef INSTALLER_RELEASE_TAG
#define INSTALLER_RELEASE_TAG "unknown"
#endif

namespace medicat {

namespace {

constexpr wchar_t kUpdateManifestUrl[] =
    L"https://raw.githubusercontent.com/mon5termatt/medicat_installer/cpp/installer/update.json";
constexpr wchar_t kGitHubReleasesApiUrl[] =
    L"https://api.github.com/repos/mon5termatt/medicat_installer/releases?per_page=10";

#if defined(_WIN64)
constexpr wchar_t kInstallerAssetName[] = L"MedicatInstaller.exe";
constexpr wchar_t kManifestAssetKey[] = L"x64";
#else
constexpr wchar_t kInstallerAssetName[] = L"MedicatInstaller-x86.exe";
constexpr wchar_t kManifestAssetKey[] = L"x86";
#endif

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), len);
    return wide;
}

std::wstring ParseJsonStringField(const std::wstring& json, const std::wstring& key) {
    const std::wstring needle = L"\"" + key + L"\":\"";
    const size_t pos = json.find(needle);
    if (pos == std::wstring::npos) {
        return {};
    }

    const size_t start = pos + needle.size();
    const size_t end = json.find(L'"', start);
    if (end == std::wstring::npos || end <= start) {
        return {};
    }
    return json.substr(start, end - start);
}

int ParseJsonIntField(const std::wstring& json, const std::wstring& key) {
    const std::wstring needle = L"\"" + key + L"\":";
    const size_t pos = json.find(needle);
    if (pos == std::wstring::npos) {
        return -1;
    }

    size_t index = pos + needle.size();
    while (index < json.size() && iswspace(json[index])) {
        ++index;
    }

    bool negative = false;
    if (index < json.size() && json[index] == L'-') {
        negative = true;
        ++index;
    }

    int value = 0;
    bool any = false;
    while (index < json.size() && iswdigit(json[index])) {
        any = true;
        value = value * 10 + (json[index] - L'0');
        ++index;
    }
    if (!any) {
        return -1;
    }
    return negative ? -value : value;
}

int ParseReleaseTagNumber(const std::wstring& tag) {
    int value = 0;
    bool any = false;
    for (const wchar_t ch : tag) {
        if (iswdigit(ch)) {
            any = true;
            value = value * 10 + (ch - L'0');
        } else if (any) {
            break;
        }
    }
    return any ? value : 0;
}

std::wstring ParseAssetUrlFromManifest(const std::wstring& json, const std::wstring& assetKey) {
    const std::wstring sectionNeedle = L"\"" + assetKey + L"\"";
    const size_t sectionPos = json.find(sectionNeedle);
    if (sectionPos == std::wstring::npos) {
        return {};
    }

    const size_t sectionEnd = json.find(L'}', sectionPos);
    const std::wstring section = sectionEnd == std::wstring::npos
                                     ? json.substr(sectionPos)
                                     : json.substr(sectionPos, sectionEnd - sectionPos);
    return ParseJsonStringField(section, L"url");
}

bool ParseManifestUpdate(const std::wstring& json, InstallerUpdateInfo& info) {
    const int remoteBuild = ParseJsonIntField(json, L"build");
    if (remoteBuild < 0) {
        return false;
    }

    info.remoteBuild = remoteBuild;
    info.version = ParseJsonStringField(json, L"version");
    info.releaseTag = ParseJsonStringField(json, L"release_tag");
    info.releaseUrl = ParseJsonStringField(json, L"release_notes_url");
    info.downloadUrl = ParseAssetUrlFromManifest(json, kManifestAssetKey);
    if (info.releaseUrl.empty() && !info.releaseTag.empty()) {
        info.releaseUrl = L"https://github.com/mon5termatt/medicat_installer/releases/tag/" + info.releaseTag;
    }
    return true;
}

bool ParseGitHubPrerelease(const std::wstring& json, InstallerUpdateInfo& info) {
    size_t pos = 0;
    while ((pos = json.find(L"\"tag_name\":\"", pos)) != std::wstring::npos) {
        const size_t blockStart = pos > 300 ? pos - 300 : 0;
        const size_t blockEnd = std::min(json.size(), pos + 4000);
        const std::wstring block = json.substr(blockStart, blockEnd - blockStart);
        if (block.find(L"\"prerelease\":true") == std::wstring::npos) {
            pos += 12;
            continue;
        }

        const size_t tagStart = pos + 12;
        const size_t tagEnd = json.find(L'"', tagStart);
        if (tagEnd == std::wstring::npos || tagEnd <= tagStart) {
            return false;
        }

        info.releaseTag = json.substr(tagStart, tagEnd - tagStart);
        info.version = info.releaseTag;
        info.releaseUrl = ParseJsonStringField(block, L"html_url");

        const std::wstring assetNeedle = L"\"name\":\"" + std::wstring(kInstallerAssetName) + L"\"";
        const size_t assetPos = block.find(assetNeedle);
        if (assetPos != std::wstring::npos) {
            const std::wstring assetSection = block.substr(assetPos, std::min<size_t>(1200, block.size() - assetPos));
            info.downloadUrl = ParseJsonStringField(assetSection, L"browser_download_url");
        }

        if (info.releaseUrl.empty() && !info.releaseTag.empty()) {
            info.releaseUrl = L"https://github.com/mon5termatt/medicat_installer/releases/tag/" + info.releaseTag;
        }
        return !info.releaseTag.empty();
    }
    return false;
}

bool IsRemoteUpdateNewer(const InstallerUpdateInfo& info) {
    if (info.remoteBuild > kInstallerBuildNumber) {
        return true;
    }

    const std::wstring localTag = Utf8ToWide(INSTALLER_RELEASE_TAG);
    const int localTagNumber = ParseReleaseTagNumber(localTag);
    const int remoteTagNumber = ParseReleaseTagNumber(info.releaseTag);
    if (remoteTagNumber > localTagNumber) {
        return true;
    }

    return false;
}

UpdateCheckResult CheckManifestUpdate() {
    UpdateCheckResult result;
    std::wstring body;
    std::wstring error;
    if (!HttpGet(kUpdateManifestUrl, body, error)) {
        result.error = error;
        return result;
    }

    if (!ParseManifestUpdate(body, result.info)) {
        result.error = L"Could not parse update manifest";
        return result;
    }

    result.info.updateAvailable = IsRemoteUpdateNewer(result.info);
    result.success = true;
    return result;
}

UpdateCheckResult CheckGitHubUpdate() {
    UpdateCheckResult result;
    std::wstring body;
    std::wstring error;
    if (!HttpGet(kGitHubReleasesApiUrl, body, error)) {
        result.error = error;
        return result;
    }

    if (!ParseGitHubPrerelease(body, result.info)) {
        result.error = L"Could not find a prerelease in GitHub releases";
        return result;
    }

    result.info.updateAvailable = IsRemoteUpdateNewer(result.info);
    result.success = true;
    return result;
}

}  // namespace

UpdateCheckResult CheckForInstallerUpdate() {
    UpdateCheckResult manifestResult = CheckManifestUpdate();
    if (manifestResult.success) {
        return manifestResult;
    }

    UpdateCheckResult githubResult = CheckGitHubUpdate();
    if (githubResult.success) {
        return githubResult;
    }

    UpdateCheckResult result;
    result.error = manifestResult.error.empty() ? githubResult.error : manifestResult.error;
    return result;
}

}  // namespace medicat
