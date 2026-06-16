#include "offline.hpp"

#include "util.hpp"

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <vector>

namespace medicat {

namespace {

std::wstring NormalizeVersion(std::wstring version) {
    while (!version.empty() && (version.front() == L'v' || version.front() == L'V')) {
        version.erase(version.begin());
    }
    return version;
}

std::wstring ReadTextFile(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return L"";
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    while (!content.empty() && (content.back() == '\r' || content.back() == '\n' || content.back() == ' ')) {
        content.pop_back();
    }
    if (content.empty()) {
        return L"";
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
    std::wstring wide(static_cast<size_t>(len > 0 ? len - 1 : 0), L'\0');
    if (len > 0) {
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, wide.data(), len);
    }
    return wide;
}

bool DirectoryExists(const std::wstring& path) {
    const DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

void EnsureDirectoryExists(const std::wstring& path) {
    if (path.empty() || DirectoryExists(path)) {
        return;
    }
    CreateDirectoryW(path.c_str(), nullptr);
}

bool ParseVentoyVersionText(const std::wstring& content, std::vector<std::wstring>& versions) {
    versions.clear();
    if (content.empty()) {
        return false;
    }

    size_t start = 0;
    while (start <= content.size()) {
        const size_t end = content.find(L'\n', start);
        const size_t lineEnd = end == std::wstring::npos ? content.size() : end;
        std::wstring line = content.substr(start, lineEnd - start);
        while (!line.empty() && (line.back() == L'\r' || line.back() == L' ' || line.back() == L'\t')) {
            line.pop_back();
        }
        const std::wstring version = NormalizeVersion(line);
        if (!version.empty()) {
            versions.push_back(version);
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }

    return !versions.empty();
}

std::vector<int> ParseVersionParts(const std::wstring& version) {
    std::vector<int> parts;
    size_t start = 0;
    while (start <= version.size()) {
        const size_t end = version.find(L'.', start);
        const size_t tokenEnd = end == std::wstring::npos ? version.size() : end;
        const std::wstring token = version.substr(start, tokenEnd - start);
        try {
            parts.push_back(token.empty() ? 0 : std::stoi(token));
        } catch (...) {
            parts.push_back(0);
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }
    return parts;
}

int CompareVersionStrings(const std::wstring& a, const std::wstring& b) {
    const std::vector<int> left = ParseVersionParts(a);
    const std::vector<int> right = ParseVersionParts(b);
    const size_t count = std::max(left.size(), right.size());
    for (size_t i = 0; i < count; ++i) {
        const int lv = i < left.size() ? left[i] : 0;
        const int rv = i < right.size() ? right[i] : 0;
        if (lv < rv) {
            return -1;
        }
        if (lv > rv) {
            return 1;
        }
    }
    return 0;
}

bool ParseVentoyZipFileName(const std::wstring& fileName, std::wstring& version) {
    const std::wstring prefix = L"ventoy-";
    const std::wstring suffix = L"-windows.zip";
    if (fileName.size() <= prefix.size() + suffix.size()) {
        return false;
    }
    if (fileName.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    if (fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) != 0) {
        return false;
    }
    version = NormalizeVersion(fileName.substr(prefix.size(), fileName.size() - prefix.size() - suffix.size()));
    return !version.empty();
}

bool LocalVentoyMatches(const std::wstring& installerRoot, const std::wstring& targetVersion) {
    const std::wstring ventoyExe = JoinPath(installerRoot, L"Ventoy2Disk\\Ventoy2Disk.exe");
    const std::wstring versionFile = JoinPath(installerRoot, L"Ventoy2Disk\\ventoy\\version");
    const std::wstring localVersion = NormalizeVersion(ReadTextFile(versionFile));
    return FileExists(ventoyExe) && !localVersion.empty() && localVersion == targetVersion;
}

}  // namespace

std::wstring GetOfflineDirectory() {
    return JoinPath(GetExeDirectory(), L"offline");
}

std::wstring GetOfflineVentoyDirectory() {
    return JoinPath(GetOfflineDirectory(), L"ventoy");
}

std::wstring GetOfflineVentoyZipPath(const std::wstring& version) {
    const std::wstring normalized = NormalizeVersion(version);
    return JoinPath(GetOfflineVentoyDirectory(), L"ventoy-" + normalized + L"-windows.zip");
}

bool OfflineVentoyZipExists(const std::wstring& version) {
    return FileExists(GetOfflineVentoyZipPath(version));
}

void CacheVentoyZip(const std::wstring& version, const std::wstring& zipPath) {
    if (version.empty() || zipPath.empty() || !FileExists(zipPath)) {
        return;
    }

    const std::wstring cacheDir = GetOfflineVentoyDirectory();
    EnsureDirectoryExists(GetOfflineDirectory());
    EnsureDirectoryExists(cacheDir);

    const std::wstring cachePath = GetOfflineVentoyZipPath(version);
    if (FileExists(cachePath)) {
        return;
    }

    CopyFileW(zipPath.c_str(), cachePath.c_str(), TRUE);
}

bool LoadOfflineVentoyVersionList(std::vector<std::wstring>& versions) {
    const std::wstring path = JoinPath(GetOfflineDirectory(), L"ventoy_versions.txt");
    return ParseVentoyVersionText(ReadTextFile(path), versions);
}

bool FindNewestCachedVentoyVersion(std::wstring& version) {
    version.clear();
    const std::wstring cacheDir = GetOfflineVentoyDirectory();
    if (!DirectoryExists(cacheDir)) {
        return false;
    }

    const std::wstring pattern = JoinPath(cacheDir, L"ventoy-*-windows.zip");
    WIN32_FIND_DATAW fd{};
    const HANDLE find = FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        std::wstring candidate;
        if (!ParseVentoyZipFileName(fd.cFileName, candidate)) {
            continue;
        }
        if (version.empty() || CompareVersionStrings(candidate, version) > 0) {
            version = candidate;
        }
    } while (FindNextFileW(find, &fd));
    FindClose(find);

    return !version.empty();
}

bool ResolveOfflineVentoyVersion(const std::wstring& pinVersion, std::wstring& version) {
    version = NormalizeVersion(pinVersion);
    if (!version.empty()) {
        return true;
    }

    std::vector<std::wstring> listed;
    if (LoadOfflineVentoyVersionList(listed)) {
        for (const std::wstring& entry : listed) {
            if (OfflineVentoyZipExists(entry)) {
                version = entry;
                return true;
            }
        }
        version = listed.front();
        return true;
    }

    return FindNewestCachedVentoyVersion(version);
}

bool CanInstallVentoyOffline(const std::wstring& installerRoot, const std::wstring& pinVersion) {
    std::wstring version;
    if (!ResolveOfflineVentoyVersion(pinVersion, version)) {
        return false;
    }

    if (LocalVentoyMatches(installerRoot, version)) {
        return true;
    }

    return OfflineVentoyZipExists(version);
}

std::wstring ResolveOfflineArchivePath(const std::wstring& archiveName) {
    const std::wstring offlinePath = JoinPath(GetOfflineDirectory(), archiveName);
    if (FileExists(offlinePath)) {
        return offlinePath;
    }
    return L"";
}

std::wstring ResolveOfflineMd5Manifest() {
    const std::wstring offlineDir = GetOfflineDirectory();
    const std::wstring candidates[] = {
        JoinPath(offlineDir, L"MedicatFiles.md5"),
        JoinPath(offlineDir, L"hasher\\MedicatFiles.md5"),
        JoinPath(offlineDir, L"hasher\\drivefiles.md5"),
    };

    for (const auto& candidate : candidates) {
        if (FileExists(candidate)) {
            return candidate;
        }
    }
    return L"";
}

}  // namespace medicat
