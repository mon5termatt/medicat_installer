#include "update.h"

#include "download.h"
#include "util.h"

#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <functional>
#include <vector>

#ifndef INSTALLER_RELEASE_TAG
#define INSTALLER_RELEASE_TAG "unknown"
#endif

namespace medicat {

namespace {

// Newest first. Prefer stable full releases that ship the C++ installer assets;
// fall back to prereleases with those assets.
constexpr wchar_t kGitHubReleasesApiUrl[] =
    L"https://api.github.com/repos/mon5termatt/medicat_installer/releases?per_page=20";

#if defined(_WIN64)
constexpr wchar_t kInstallerAssetName[] = L"MedicatInstaller.exe";
#else
constexpr wchar_t kInstallerAssetName[] = L"MedicatInstaller-x86.exe";
#endif

std::wstring Utf8ToWideLocal(const std::string& text) {
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

std::wstring ParseJsonStringField(const std::wstring& json, const std::wstring& key, const size_t from = 0) {
    const std::wstring needle = L"\"" + key + L"\"";
    const size_t pos = json.find(needle, from);
    if (pos == std::wstring::npos) {
        return {};
    }

    size_t index = pos + needle.size();
    while (index < json.size() && iswspace(json[index])) {
        ++index;
    }
    if (index >= json.size() || json[index] != L':') {
        return {};
    }
    ++index;
    while (index < json.size() && iswspace(json[index])) {
        ++index;
    }
    if (index >= json.size() || json[index] != L'"') {
        return {};
    }
    ++index;

    std::wstring out;
    while (index < json.size()) {
        const wchar_t ch = json[index++];
        if (ch == L'\\') {
            if (index >= json.size()) {
                break;
            }
            const wchar_t esc = json[index++];
            switch (esc) {
                case L'"':
                case L'\\':
                case L'/':
                    out.push_back(esc);
                    break;
                case L'n':
                    out.push_back(L'\n');
                    break;
                case L'r':
                    out.push_back(L'\r');
                    break;
                case L't':
                    out.push_back(L'\t');
                    break;
                case L'u':
                    // Skip unicode escapes (four hex digits).
                    for (int i = 0; i < 4 && index < json.size(); ++i) {
                        ++index;
                    }
                    break;
                default:
                    out.push_back(esc);
                    break;
            }
            continue;
        }
        if (ch == L'"') {
            break;
        }
        out.push_back(ch);
    }
    return out;
}

bool ParseJsonBoolField(const std::wstring& json, const std::wstring& key, bool& value) {
    const std::wstring needle = L"\"" + key + L"\"";
    const size_t pos = json.find(needle);
    if (pos == std::wstring::npos) {
        return false;
    }

    size_t index = pos + needle.size();
    while (index < json.size() && iswspace(json[index])) {
        ++index;
    }
    if (index >= json.size() || json[index] != L':') {
        return false;
    }
    ++index;
    while (index < json.size() && iswspace(json[index])) {
        ++index;
    }

    if (json.compare(index, 4, L"true") == 0) {
        value = true;
        return true;
    }
    if (json.compare(index, 5, L"false") == 0) {
        value = false;
        return true;
    }
    return false;
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

struct SemVer {
    int major = 0;
    int minor = 0;
    int patch = 0;
    bool ok = false;
};

SemVer ParseSemVer(const std::wstring& tagIn) {
    SemVer v{};
    std::wstring tag = tagIn;
    if (!tag.empty() && (tag[0] == L'v' || tag[0] == L'V')) {
        tag.erase(tag.begin());
    }
    const size_t dash = tag.find(L'-');
    if (dash != std::wstring::npos) {
        tag = tag.substr(0, dash);
    }
    const size_t plus = tag.find(L'+');
    if (plus != std::wstring::npos) {
        tag = tag.substr(0, plus);
    }

    int parts[3] = {0, 0, 0};
    int part = 0;
    size_t i = 0;
    bool anyDigit = false;
    while (i < tag.size() && part < 3) {
        if (!iswdigit(tag[i])) {
            if (tag[i] == L'.' && anyDigit) {
                ++part;
                anyDigit = false;
                ++i;
                continue;
            }
            break;
        }
        anyDigit = true;
        parts[part] = parts[part] * 10 + (tag[i] - L'0');
        ++i;
    }
    // Require major.minor.patch (two dots / three numeric components).
    if (part < 2 || !anyDigit) {
        return v;
    }
    v.major = parts[0];
    v.minor = parts[1];
    v.patch = parts[2];
    v.ok = true;
    return v;
}

int CompareSemVer(const SemVer& a, const SemVer& b) {
    if (a.major != b.major) {
        return a.major < b.major ? -1 : 1;
    }
    if (a.minor != b.minor) {
        return a.minor < b.minor ? -1 : 1;
    }
    if (a.patch != b.patch) {
        return a.patch < b.patch ? -1 : 1;
    }
    return 0;
}

std::wstring FindAssetDownloadUrl(const std::wstring& block, const std::wstring& assetName) {
    size_t from = 0;
    while (from < block.size()) {
        const size_t pos = block.find(L"\"name\"", from);
        if (pos == std::wstring::npos) {
            break;
        }

        const std::wstring name = ParseJsonStringField(block, L"name", pos);
        if (name == assetName) {
            const size_t assetStart = block.rfind(L'{', pos);
            const size_t searchEnd = std::min(block.size(), pos + 8000);
            const std::wstring section =
                assetStart == std::wstring::npos ? block.substr(0, searchEnd)
                                                 : block.substr(assetStart, searchEnd - assetStart);
            const std::wstring url = ParseJsonStringField(section, L"browser_download_url");
            if (!url.empty()) {
                return url;
            }
        }

        from = pos + 6;
    }
    return {};
}

struct ParsedRelease {
    bool prerelease = false;
    std::wstring tag;
    std::wstring htmlUrl;
    std::wstring downloadUrl;
    std::wstring name;
};

bool ParseReleaseBlock(const std::wstring& block, ParsedRelease& out) {
    out = {};
    if (!ParseJsonBoolField(block, L"prerelease", out.prerelease)) {
        out.prerelease = false;
    }
    bool draft = false;
    if (ParseJsonBoolField(block, L"draft", draft) && draft) {
        return false;
    }

    out.tag = ParseJsonStringField(block, L"tag_name");
    out.htmlUrl = ParseJsonStringField(block, L"html_url");
    out.name = ParseJsonStringField(block, L"name");
    out.downloadUrl = FindAssetDownloadUrl(block, kInstallerAssetName);
    return !out.tag.empty() && !out.downloadUrl.empty();
}

// Walk release objects (API returns newest first). Call visitor(block); return true to stop.
void ForEachReleaseBlock(const std::wstring& json, const std::function<bool(const std::wstring&)>& visitor) {
    size_t from = 0;
    while (from < json.size()) {
        const size_t tagKeyPos = json.find(L"\"tag_name\"", from);
        if (tagKeyPos == std::wstring::npos) {
            break;
        }

        const size_t blockStart = tagKeyPos > 0 ? json.rfind(L'{', tagKeyPos) : 0;
        const size_t nextTag = json.find(L"\"tag_name\"", tagKeyPos + 10);
        const size_t blockEnd = nextTag == std::wstring::npos ? json.size() : nextTag;
        const std::wstring block =
            blockStart == std::wstring::npos ? json.substr(0, blockEnd) : json.substr(blockStart, blockEnd - blockStart);

        if (visitor(block)) {
            return;
        }
        from = tagKeyPos + 10;
    }
}

bool FindBestInstallerRelease(const std::wstring& json, ParsedRelease& best) {
    ParsedRelease firstStableSemver{};
    ParsedRelease firstAnySemver{};
    ParsedRelease firstStable{};
    ParsedRelease firstPrerelease{};
    bool haveStableSemver = false;
    bool haveAnySemver = false;
    bool haveStable = false;
    bool havePrerelease = false;

    ForEachReleaseBlock(json, [&](const std::wstring& block) {
        ParsedRelease release{};
        if (!ParseReleaseBlock(block, release)) {
            return false;
        }
        const bool semver = ParseSemVer(release.tag).ok;
        if (semver) {
            if (!haveAnySemver) {
                firstAnySemver = release;
                haveAnySemver = true;
            }
            if (!release.prerelease && !haveStableSemver) {
                firstStableSemver = release;
                haveStableSemver = true;
            }
        }
        if (!release.prerelease) {
            if (!haveStable) {
                firstStable = release;
                haveStable = true;
            }
        } else if (!havePrerelease) {
            firstPrerelease = release;
            havePrerelease = true;
        }
        return haveStableSemver;
    });

    if (haveStableSemver) {
        best = firstStableSemver;
        return true;
    }
    if (haveAnySemver) {
        best = firstAnySemver;
        return true;
    }
    if (haveStable) {
        best = firstStable;
        return true;
    }
    if (havePrerelease) {
        best = firstPrerelease;
        return true;
    }
    return false;
}

bool IsRemoteUpdateNewer(const InstallerUpdateInfo& info) {
    // Prefer patch/build compare — always matches the displayed kInstallerVersion.
    if (info.remoteBuild > 0 && info.remoteBuild > kInstallerBuildNumber) {
        return true;
    }

    const std::wstring localVersion = Utf8ToWideLocal(kInstallerVersion);
    const std::wstring localTag = [&]() {
        const std::wstring fromDefine = Utf8ToWideLocal(INSTALLER_RELEASE_TAG);
        if (!fromDefine.empty() && _wcsicmp(fromDefine.c_str(), L"unknown") != 0) {
            return fromDefine;
        }
        return localVersion;
    }();

    if (info.releaseTag.empty()) {
        return false;
    }
    if (!localTag.empty() && _wcsicmp(info.releaseTag.c_str(), localTag.c_str()) == 0) {
        return false;
    }
    if (!localVersion.empty() && _wcsicmp(info.releaseTag.c_str(), localVersion.c_str()) == 0) {
        return false;
    }

    const SemVer localSem = ParseSemVer(!localVersion.empty() ? localVersion : localTag);
    const SemVer remoteSem = ParseSemVer(info.releaseTag);

    // New scheme: tags are 1.0.N. Do not "update" to legacy numeric tags (3520 / 3521-BETA).
    if (localSem.ok && !remoteSem.ok) {
        return false;
    }
    if (!localSem.ok && remoteSem.ok) {
        return true;
    }
    if (localSem.ok && remoteSem.ok) {
        return CompareSemVer(remoteSem, localSem) > 0;
    }

    // Both legacy-style tags (old batch scheme) — keep prior behavior for transition.
    const int localTagNumber = ParseReleaseTagNumber(localTag);
    const int remoteTagNumber = ParseReleaseTagNumber(info.releaseTag);
    if (remoteTagNumber > localTagNumber) {
        return true;
    }
    if (remoteTagNumber == localTagNumber && remoteTagNumber > 0) {
        const bool localBeta = localTag.find(L'-') != std::wstring::npos;
        const bool remoteBeta = info.releaseTag.find(L'-') != std::wstring::npos;
        if (localBeta && !remoteBeta) {
            return true;
        }
        if (!localBeta && remoteBeta) {
            return false;
        }
        return _wcsicmp(info.releaseTag.c_str(), localTag.c_str()) > 0;
    }
    return false;
}

}  // namespace

std::wstring GetRunningInstallerExePath() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }
    return std::wstring(buffer, length);
}

std::wstring QuoteCmdArgument(const std::wstring& value) {
    if (value.find_first_of(L" \t\"") == std::wstring::npos) {
        return value;
    }

    std::wstring quoted = L"\"";
    for (const wchar_t ch : value) {
        if (ch == L'"') {
            quoted += L"\\\"";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back(L'"');
    return quoted;
}

bool WriteUtf8TextFile(const std::wstring& path, const std::string& content) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = WriteFile(file, content.data(), static_cast<DWORD>(content.size()), &written, nullptr);
    CloseHandle(file);
    return ok && written == content.size();
}

bool LaunchUpdateReplacer(const std::wstring& downloadedPath, const std::wstring& targetPath, DWORD parentPid,
                          std::wstring& error, const std::function<void(const std::wstring&)>& onLog) {
    (void)parentPid;

    const std::wstring exeDir = GetExeDirectory();
    const std::wstring tempDir = GetMedicatTempDir();
    const std::wstring quotedNew = QuoteCmdArgument(downloadedPath);
    const std::wstring quotedTarget = QuoteCmdArgument(targetPath);
    const std::wstring quotedExeDir = QuoteCmdArgument(exeDir);

    std::wstring inner = L"@echo off\r\n";
    inner += L"setlocal EnableExtensions EnableDelayedExpansion\r\n";
    inner += L"ping 127.0.0.1 -n 3 >nul\r\n";
    inner += L"set RETRIES=0\r\n";
    inner += L":retry\r\n";
    inner += L"move /Y " + quotedNew + L" " + quotedTarget + L" >nul 2>&1\r\n";
    inner += L"if not errorlevel 1 goto launch\r\n";
    inner += L"set /a RETRIES+=1\r\n";
    inner += L"if !RETRIES! geq 15 exit /b 1\r\n";
    inner += L"ping 127.0.0.1 -n 2 >nul\r\n";
    inner += L"goto retry\r\n";
    inner += L":launch\r\n";
    inner += L"start \"\" /D " + quotedExeDir + L" " + quotedTarget + L"\r\n";
    inner += L"exit /b 0\r\n";

    const std::wstring batchPath = JoinPath(tempDir, L"apply_update.cmd");
    if (!WriteUtf8TextFile(batchPath, WideToUtf8(inner))) {
        error = L"Could not create update helper script";
        return false;
    }

    std::wstring command = L"cmd.exe /c " + QuoteCmdArgument(batchPath);

    if (onLog) {
        onLog(L"[Update] Launching helper: " + command);
    }

    std::vector<wchar_t> commandBuffer(command.begin(), command.end());
    commandBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(nullptr, commandBuffer.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        exeDir.c_str(), &startupInfo, &processInfo)) {
        DeleteFileW(batchPath.c_str());
        error = L"Could not launch update helper (error " + std::to_wstring(GetLastError()) + L")";
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    if (onLog) {
        onLog(L"[Update] Helper started (pid " + std::to_wstring(processInfo.dwProcessId) + L")");
    }

    Sleep(1000);
    return true;
}

std::wstring GetInstallerAssetFileName() {
#if defined(_WIN64)
    return L"MedicatInstaller.exe";
#else
    return L"MedicatInstaller-x86.exe";
#endif
}

bool DownloadAndRelaunchInstallerUpdate(
    const InstallerUpdateInfo& info, const std::function<void(uint64_t downloaded, uint64_t total)>& onProgress,
    const std::function<void(const std::wstring&)>& onLog, std::wstring& error) {
    if (info.downloadUrl.empty()) {
        error = L"No download URL for installer update";
        return false;
    }

    const std::wstring targetPath = GetRunningInstallerExePath();
    if (targetPath.empty()) {
        error = L"Could not determine running installer path";
        return false;
    }

    const std::wstring downloadedPath = targetPath + L".new";
    DeleteFileW(downloadedPath.c_str());

    if (onLog) {
        onLog(L"[Update] Downloading " + info.downloadUrl + L" -> " + downloadedPath);
    }

    if (!HttpDownloadFileWithProgress(info.downloadUrl, downloadedPath, onProgress, error)) {
        DeleteFileW(downloadedPath.c_str());
        if (onLog) {
            onLog(L"[Update] Download failed — " + error);
        }
        return false;
    }

    constexpr uint64_t kMinInstallerBytes = 256 * 1024;
    const uint64_t downloadedSize = GetFileSizeBytes(downloadedPath);
    if (downloadedSize < kMinInstallerBytes) {
        DeleteFileW(downloadedPath.c_str());
        error = L"Downloaded file is too small (" + FormatBytes(downloadedSize) + L")";
        if (onLog) {
            onLog(L"[Update] " + error);
        }
        return false;
    }

    if (onLog) {
        onLog(L"[Update] Download complete (" + FormatBytes(downloadedSize) + L") — applying update");
    }

    const DWORD parentPid = GetCurrentProcessId();
    if (!LaunchUpdateReplacer(downloadedPath, targetPath, parentPid, error, onLog)) {
        DeleteFileW(downloadedPath.c_str());
        if (onLog) {
            onLog(L"[Update] Relaunch helper failed — " + error);
        }
        return false;
    }

    return true;
}

UpdateCheckResult CheckForInstallerUpdate() {
    UpdateCheckResult result;
    std::wstring body;
    std::wstring error;
    if (!HttpGet(kGitHubReleasesApiUrl, body, error)) {
        result.error = error.empty() ? L"Could not query GitHub releases" : error;
        return result;
    }

    ParsedRelease release{};
    if (!FindBestInstallerRelease(body, release)) {
        result.error = L"No GitHub release publishes " + GetInstallerAssetFileName();
        return result;
    }

    result.info.releaseTag = release.tag;
    result.info.version = !release.name.empty() ? release.name : release.tag;
    // Prefer clean semver for display when the tag is 1.0.N
    {
        const SemVer tagVer = ParseSemVer(release.tag);
        if (tagVer.ok) {
            result.info.version =
                std::to_wstring(tagVer.major) + L"." + std::to_wstring(tagVer.minor) + L"." +
                std::to_wstring(tagVer.patch);
            result.info.remoteBuild = tagVer.patch;
        }
    }
    result.info.releaseUrl = release.htmlUrl;
    if (result.info.releaseUrl.empty()) {
        result.info.releaseUrl = L"https://github.com/mon5termatt/medicat_installer/releases/tag/" + release.tag;
    }
    result.info.downloadUrl = release.downloadUrl;
    result.info.updateAvailable = IsRemoteUpdateNewer(result.info);
    result.success = true;
    return result;
}

}  // namespace medicat
