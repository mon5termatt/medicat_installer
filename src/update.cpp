#include "update.h"

#include "download.h"
#include "util.h"

#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <vector>

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

    const size_t start = index;
    const size_t end = json.find(L'"', start);
    if (end == std::wstring::npos || end <= start) {
        return {};
    }
    return json.substr(start, end - start);
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
            const size_t searchEnd = std::min(block.size(), pos + 6000);
            const std::wstring section = block.substr(assetStart, searchEnd - assetStart);
            const std::wstring url = ParseJsonStringField(section, L"browser_download_url");
            if (!url.empty()) {
                return url;
            }
        }

        from = pos + 6;
    }
    return {};
}

bool ParseGitHubPrerelease(const std::wstring& json, InstallerUpdateInfo& info) {
    size_t from = 0;
    while (from < json.size()) {
        const size_t tagKeyPos = json.find(L"\"tag_name\"", from);
        if (tagKeyPos == std::wstring::npos) {
            break;
        }

        const size_t blockStart = tagKeyPos > 0 ? json.rfind(L'{', tagKeyPos) : 0;
        const size_t nextTag = json.find(L"\"tag_name\"", tagKeyPos + 10);
        const size_t blockEnd = nextTag == std::wstring::npos ? json.size() : nextTag;
        const std::wstring block = json.substr(blockStart, blockEnd - blockStart);

        bool prerelease = false;
        if (!ParseJsonBoolField(block, L"prerelease", prerelease) || !prerelease) {
            from = tagKeyPos + 10;
            continue;
        }

        info.releaseTag = ParseJsonStringField(block, L"tag_name");
        info.version = info.releaseTag;
        info.releaseUrl = ParseJsonStringField(block, L"html_url");
        info.downloadUrl = FindAssetDownloadUrl(block, kInstallerAssetName);

        if (info.releaseUrl.empty() && !info.releaseTag.empty()) {
            info.releaseUrl = L"https://github.com/mon5termatt/medicat_installer/releases/tag/" + info.releaseTag;
        }
        return !info.releaseTag.empty() && !info.downloadUrl.empty();
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
        error = L"No download URL in update manifest";
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
    UpdateCheckResult githubResult = CheckGitHubUpdate();
    if (githubResult.success && !githubResult.info.downloadUrl.empty()) {
        const UpdateCheckResult manifestResult = CheckManifestUpdate();
        if (manifestResult.success) {
            if (!manifestResult.info.version.empty()) {
                githubResult.info.version = manifestResult.info.version;
            }
            if (manifestResult.info.remoteBuild > 0) {
                githubResult.info.remoteBuild = manifestResult.info.remoteBuild;
            }
            githubResult.info.updateAvailable = IsRemoteUpdateNewer(githubResult.info);
        }
        return githubResult;
    }

    UpdateCheckResult manifestResult = CheckManifestUpdate();
    if (manifestResult.success) {
        return manifestResult;
    }

    UpdateCheckResult result;
    result.error = githubResult.error.empty() ? manifestResult.error : githubResult.error;
    return result;
}

}  // namespace medicat
