#include "update_pack.h"

#include "downloads.h"
#include "offline.h"
#include "util.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <fstream>
#include <map>
#include <sstream>

namespace medicat {

namespace {

std::wstring NormalizePackRelPath(std::wstring path) {
    while (!path.empty() && (path.front() == L'\\' || path.front() == L'/')) {
        path.erase(path.begin());
    }
    for (wchar_t& ch : path) {
        if (ch == L'\\') {
            ch = L'/';
        }
    }
    while (!path.empty() && path.back() == L'/') {
        path.pop_back();
    }
    return path;
}

std::wstring ToLowerCopy(std::wstring value) {
    for (wchar_t& ch : value) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return value;
}

bool IsHexMd5(const std::string& value) {
    if (value.size() != 32) {
        return false;
    }
    for (const unsigned char ch : value) {
        if (!std::isxdigit(ch)) {
            return false;
        }
    }
    return true;
}

std::string ToLowerAscii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool WriteFileUtf8(const std::wstring& path, const std::string& contents, std::wstring& error) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = L"Could not write file: " + path;
        return false;
    }
    DWORD written = 0;
    if (!contents.empty() &&
        (!WriteFile(file, contents.data(), static_cast<DWORD>(contents.size()), &written, nullptr) ||
         written != contents.size())) {
        CloseHandle(file);
        error = L"Failed writing file: " + path;
        return false;
    }
    CloseHandle(file);
    return true;
}

bool ReadFileUtf8(const std::wstring& path, std::string& out, std::wstring& error) {
    out.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = L"Could not open file: " + path;
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 8 * 1024 * 1024) {
        CloseHandle(file);
        error = L"Invalid file size: " + path;
        return false;
    }
    out.resize(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    if (size.QuadPart > 0 &&
        (!ReadFile(file, out.data(), static_cast<DWORD>(out.size()), &read, nullptr) || read != out.size())) {
        CloseHandle(file);
        error = L"Failed reading file: " + path;
        return false;
    }
    CloseHandle(file);
    if (!out.empty() && static_cast<unsigned char>(out[0]) == 0xEF && out.size() >= 3 &&
        static_cast<unsigned char>(out[1]) == 0xBB && static_cast<unsigned char>(out[2]) == 0xBF) {
        out.erase(0, 3);
    }
    return true;
}

bool ExtractJsonStringField(const std::string& json, const char* key, std::string& value) {
    value.clear();
    const std::string needle = std::string("\"") + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return false;
    }
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') {
        return false;
    }
    ++pos;
    std::string out;
    while (pos < json.size()) {
        const char ch = json[pos++];
        if (ch == '\\' && pos < json.size()) {
            const char esc = json[pos++];
            if (esc == '"' || esc == '\\' || esc == '/') {
                out.push_back(esc);
            } else if (esc == 'n') {
                out.push_back('\n');
            } else if (esc == 'r') {
                out.push_back('\r');
            } else if (esc == 't') {
                out.push_back('\t');
            } else if (esc == 'b') {
                out.push_back('\b');
            } else if (esc == 'f') {
                out.push_back('\f');
            } else {
                out.push_back(esc);
            }
            continue;
        }
        if (ch == '"') {
            value = out;
            return true;
        }
        out.push_back(ch);
    }
    return false;
}

bool ExtractJsonBoolField(const std::string& json, const char* key, bool& value) {
    value = false;
    const std::string needle = std::string("\"") + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return false;
    }
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        ++pos;
    }
    if (pos + 4 <= json.size() && json.compare(pos, 4, "true") == 0) {
        value = true;
        return true;
    }
    if (pos + 5 <= json.size() && json.compare(pos, 5, "false") == 0) {
        value = false;
        return true;
    }
    return false;
}

bool IsBootablesProtectedRootFile(const std::wstring& relative) {
    const std::wstring normalized = ToLowerCopy(NormalizePackRelPath(relative));
    if (normalized.empty() || normalized.find(L'/') != std::wstring::npos) {
        return false;
    }
    return normalized == L"start.exe" || normalized == L"cdusb.y" || normalized == L"license.txt" ||
           normalized == L"licence.txt" || normalized == L"autorun.inf" || normalized == L"autorun.ico";
}

bool IsBootablesProtectedPath(const std::wstring& relative) {
    const std::wstring normalized = ToLowerCopy(NormalizePackRelPath(relative));
    if (normalized == L"portableapps" || normalized.rfind(L"portableapps/", 0) == 0 ||
        normalized == L"programs" || normalized.rfind(L"programs/", 0) == 0) {
        return true;
    }
    return IsBootablesProtectedRootFile(relative);
}

bool ExtractJsonStringArrayField(const std::string& json, const char* key, std::vector<std::wstring>& values) {
    values.clear();
    const std::string needle = std::string("\"") + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return false;
    }
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '[') {
        return false;
    }
    ++pos;
    while (pos < json.size()) {
        while (pos < json.size() &&
               (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n' || json[pos] == ',')) {
            ++pos;
        }
        if (pos < json.size() && json[pos] == ']') {
            return true;
        }
        if (pos >= json.size() || json[pos] != '"') {
            return false;
        }
        ++pos;
        std::string item;
        while (pos < json.size()) {
            const char ch = json[pos++];
            if (ch == '\\' && pos < json.size()) {
                item.push_back(json[pos++]);
                continue;
            }
            if (ch == '"') {
                break;
            }
            item.push_back(ch);
        }
        values.push_back(NormalizePackRelPath(Utf8ToWide(item)));
    }
    return false;
}

bool GlobEndsWithUpdate7z(const std::wstring& fileName) {
    const std::wstring lower = ToLowerCopy(fileName);
    return lower.size() >= 9 && lower.compare(lower.size() - 9, 9, L"update.7z") == 0;
}

bool GlobStartsWithMedicatUsb(const std::wstring& fileName) {
    const std::wstring lower = ToLowerCopy(fileName);
    return lower.rfind(L"medicat.usb.", 0) == 0;
}

void CollectUpdatePackCandidates(const std::wstring& directory, std::vector<std::wstring>& outPaths) {
    if (directory.empty()) {
        return;
    }
    const std::wstring pattern = JoinPath(directory, kMediCatUpdatePackGlob);
    WIN32_FIND_DATAW findData{};
    HANDLE find = FindFirstFileW(pattern.c_str(), &findData);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        const std::wstring name = findData.cFileName;
        if (!MatchesUpdatePackGlob(name)) {
            continue;
        }
        outPaths.push_back(JoinPath(directory, name));
    } while (FindNextFileW(find, &findData));
    FindClose(find);
}

uint64_t FileLastWrite100ns(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        return 0;
    }
    ULARGE_INTEGER value{};
    value.LowPart = data.ftLastWriteTime.dwLowDateTime;
    value.HighPart = data.ftLastWriteTime.dwHighDateTime;
    return value.QuadPart;
}

bool EnsureDirectoryDeep(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    // Drive roots (H:\) already exist; CreateDirectoryW returns Access Denied, not
    // ERROR_ALREADY_EXISTS, so treat an existing directory as success first.
    std::wstring normalized = path;
    while (normalized.size() > 3 && (normalized.back() == L'\\' || normalized.back() == L'/')) {
        normalized.pop_back();
    }
    if (normalized.size() == 2 && normalized[1] == L':') {
        normalized.push_back(L'\\');
    }
    const DWORD existing = GetFileAttributesW(normalized.c_str());
    if (existing != INVALID_FILE_ATTRIBUTES && (existing & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return true;
    }

    if (CreateDirectoryW(normalized.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }
    const size_t slash = normalized.find_last_of(L"\\/");
    if (slash == std::wstring::npos || slash < 2) {
        return false;
    }
    if (!EnsureDirectoryDeep(normalized.substr(0, slash))) {
        return false;
    }
    return CreateDirectoryW(normalized.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool IsDriveRootPath(std::wstring path) {
    while (!path.empty() && (path.back() == L'\\' || path.back() == L'/')) {
        path.pop_back();
    }
    return path.size() == 2 && path[1] == L':';
}

bool DirectoryHasChildren(const std::wstring& path) {
    const std::wstring pattern = JoinPath(path, L"*");
    WIN32_FIND_DATAW findData{};
    HANDLE find = FindFirstFileW(pattern.c_str(), &findData);
    if (find == INVALID_HANDLE_VALUE) {
        return false;
    }
    bool hasChildren = false;
    do {
        const std::wstring name = findData.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        hasChildren = true;
        break;
    } while (FindNextFileW(find, &findData));
    FindClose(find);
    return hasChildren;
}

// Remove empty parent folders up to (but not including) driveRoot after a file/dir delete.
void PruneEmptyParentDirs(const std::wstring& deletedPath, const std::wstring& driveRoot,
                          const std::function<void(const std::wstring&)>& onLog) {
    std::wstring current = deletedPath;
    for (;;) {
        const size_t slash = current.find_last_of(L"\\/");
        if (slash == std::wstring::npos || slash < 2) {
            break;
        }
        current = current.substr(0, slash);
        if (current.size() == 2 && current[1] == L':') {
            break;
        }
        if (IsDriveRootPath(current) || ToLowerCopy(current) == ToLowerCopy(driveRoot)) {
            break;
        }
        // Stay under the destination drive root.
        std::wstring root = driveRoot;
        while (!root.empty() && (root.back() == L'\\' || root.back() == L'/')) {
            root.pop_back();
        }
        if (ToLowerCopy(current).rfind(ToLowerCopy(root), 0) != 0) {
            break;
        }

        const DWORD attrs = GetFileAttributesW(current.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            break;
        }
        if (DirectoryHasChildren(current)) {
            break;
        }
        if (RemoveDirectoryW(current.c_str()) == 0) {
            break;
        }
        if (onLog) {
            onLog(L"Update pack pruned empty dir: " + current);
        }
    }
}

bool DeletePathRecursive(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return true;
    }
    if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
        return DeleteFileW(path.c_str()) != 0;
    }

    const std::wstring pattern = JoinPath(path, L"*");
    WIN32_FIND_DATAW findData{};
    HANDLE find = FindFirstFileW(pattern.c_str(), &findData);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            const std::wstring name = findData.cFileName;
            if (name == L"." || name == L"..") {
                continue;
            }
            DeletePathRecursive(JoinPath(path, name));
        } while (FindNextFileW(find, &findData));
        FindClose(find);
    }
    return RemoveDirectoryW(path.c_str()) != 0;
}

bool PathExistsUnder(const std::wstring& root, const std::wstring& relative) {
    std::wstring rel = relative;
    for (wchar_t& ch : rel) {
        if (ch == L'/') {
            ch = L'\\';
        }
    }
    return FileExists(JoinPath(root, rel)) ||
           (GetFileAttributesW(JoinPath(root, rel).c_str()) != INVALID_FILE_ATTRIBUTES);
}

}  // namespace

bool MatchesUpdatePackGlob(const std::wstring& fileName) {
    return GlobStartsWithMedicatUsb(fileName) && GlobEndsWithUpdate7z(fileName);
}

UpdatePackResolveResult ResolveUpdatePackPath(const std::wstring& installerRoot,
                                              const std::wstring& browseOverride) {
    UpdatePackResolveResult result;
    if (!browseOverride.empty()) {
        result.found = FileExists(browseOverride);
        result.path = browseOverride;
        result.candidates.push_back(browseOverride);
        return result;
    }

    CollectUpdatePackCandidates(installerRoot, result.candidates);
    CollectUpdatePackCandidates(GetOfflineDirectory(), result.candidates);

    if (result.candidates.empty()) {
        return result;
    }

    std::stable_sort(result.candidates.begin(), result.candidates.end(),
                     [](const std::wstring& a, const std::wstring& b) {
                         return FileLastWrite100ns(a) > FileLastWrite100ns(b);
                     });
    result.found = true;
    result.path = result.candidates.front();
    return result;
}

bool ParseUpdatePackInfoJson(const std::string& jsonUtf8, UpdatePackInfo& info, std::wstring& error) {
    error.clear();
    std::string name;
    std::string version;
    std::string base;
    std::string contentRoot;
    std::string manifest;
    std::string archiveMd5;

    ExtractJsonStringField(jsonUtf8, "name", name);
    if (!ExtractJsonStringField(jsonUtf8, "version", version) || version.empty()) {
        error = L"info.json missing required string field: version";
        return false;
    }
    if (!ExtractJsonStringField(jsonUtf8, "base", base) || base.empty()) {
        error = L"info.json missing required string field: base";
        return false;
    }
    if (!ExtractJsonStringField(jsonUtf8, "contentRoot", contentRoot)) {
        error = L"info.json missing required string field: contentRoot";
        return false;
    }
    if (contentRoot.empty()) {
        contentRoot = ".";
    }
    if (contentRoot != ".") {
        error = L"info.json contentRoot must be \".\" (flat USB-relative archive layout)";
        return false;
    }
    if (!ExtractJsonStringField(jsonUtf8, "manifest", manifest) || manifest.empty()) {
        error = L"info.json missing required string field: manifest";
        return false;
    }
    if (!ExtractJsonStringField(jsonUtf8, "archiveMd5", archiveMd5) || !IsHexMd5(archiveMd5)) {
        error = L"info.json missing or invalid archiveMd5 (expected 32 hex chars)";
        return false;
    }

    std::vector<std::wstring> removePaths;
    std::vector<std::wstring> addPaths;
    if (!ExtractJsonStringArrayField(jsonUtf8, "remove", removePaths)) {
        error = L"info.json missing required array field: remove";
        return false;
    }
    if (!ExtractJsonStringArrayField(jsonUtf8, "add", addPaths)) {
        error = L"info.json missing required array field: add";
        return false;
    }

    info.name = Utf8ToWide(name);
    info.version = Utf8ToWide(version);
    info.base = Utf8ToWide(base);
    info.contentRoot = NormalizePackRelPath(Utf8ToWide(contentRoot));
    info.manifest = NormalizePackRelPath(Utf8ToWide(manifest));
    info.archiveMd5 = ToLowerAscii(archiveMd5);
    std::string md5Manifest;
    ExtractJsonStringField(jsonUtf8, "md5Manifest", md5Manifest);
    info.md5Manifest = std::move(md5Manifest);
    bool bootablesOnly = false;
    ExtractJsonBoolField(jsonUtf8, "bootablesOnly", bootablesOnly);
    info.bootablesOnly = bootablesOnly;
    info.remove = std::move(removePaths);
    info.add = std::move(addPaths);

    if (info.manifest.find(L'/') != std::wstring::npos) {
        error = L"info.json manifest must be a file name in the archive root";
        return false;
    }

    // bootablesOnly packs must not touch PortableApps/Programs or USB chrome root files.
    if (info.bootablesOnly) {
        for (const std::wstring& path : info.remove) {
            if (IsBootablesProtectedPath(path)) {
                error = L"bootablesOnly pack must not list protected path in remove: " + path;
                return false;
            }
        }
        for (const std::wstring& path : info.add) {
            if (IsBootablesProtectedPath(path) && !IsBootablesProtectedRootFile(path)) {
                error = L"bootablesOnly pack must not list PortableApps/Programs in add: " + path;
                return false;
            }
        }
    }
    return true;
}

std::wstring ReplaceArchiveExtension(const std::wstring& archivePath, const std::wstring& newExt) {
    // MediCat.USB.v26.12.update.7z -> MediCat.USB.v26.12.update.json / .md5manifest
    const size_t slash = archivePath.find_last_of(L"\\/");
    const size_t dot = archivePath.find_last_of(L'.');
    if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash)) {
        return archivePath + newExt;
    }
    return archivePath.substr(0, dot) + newExt;
}

std::wstring ParentOfPath(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    if (slash == 0) {
        return path.substr(0, 1);
    }
    return path.substr(0, slash);
}

bool CopyFileToWorkDir(const std::wstring& src, const std::wstring& dest, std::wstring& error) {
    if (!CopyFileW(src.c_str(), dest.c_str(), FALSE)) {
        error = L"Could not copy sidecar to work dir: " + src;
        return false;
    }
    return true;
}

bool LoadUpdatePackInfo(const std::wstring& sevenZipExe, const std::wstring& archivePath,
                        const std::wstring& workDir, UpdatePackInfo& info, std::wstring& error,
                        const std::wstring& extractLogPath) {
    error.clear();
    info = {};
    info.archivePath = archivePath;

    if (archivePath.empty() || !FileExists(archivePath)) {
        error = L"Update pack archive not found";
        return false;
    }
    EnsureDirectoryDeep(workDir);

    const std::wstring infoPath = JoinPath(workDir, L"info.json");
    const std::wstring sidecarJson = ReplaceArchiveExtension(archivePath, L".json");
    const std::wstring legacySidecarJson = ReplaceArchiveExtension(archivePath, L".info.json");
    bool loadedJsonFromSidecar = false;
    if (FileExists(sidecarJson)) {
        if (!CopyFileToWorkDir(sidecarJson, infoPath, error)) {
            return false;
        }
        loadedJsonFromSidecar = true;
    } else if (FileExists(legacySidecarJson)) {
        if (!CopyFileToWorkDir(legacySidecarJson, infoPath, error)) {
            return false;
        }
        loadedJsonFromSidecar = true;
    } else {
        if (sevenZipExe.empty() || !FileExists(sevenZipExe)) {
            error = L"7za.exe not available for update pack inspection (no sidecar .json found)";
            return false;
        }
        const std::vector<std::wstring> metaMembers = {L"info.json"};
        const ExtractResult metaExtract =
            Extract7zArchiveSelective(sevenZipExe, archivePath, workDir, metaMembers, {}, extractLogPath, false);
        if (!metaExtract.success) {
            error = L"Could not extract info.json from update pack: " + FormatExtractFailureMessage(metaExtract);
            return false;
        }
    }

    std::string json;
    if (!ReadFileUtf8(infoPath, json, error)) {
        return false;
    }
    if (!ParseUpdatePackInfoJson(json, info, error)) {
        return false;
    }
    info.archivePath = archivePath;

    const std::wstring manifestDest = JoinPath(workDir, info.manifest);
    if (!info.md5Manifest.empty()) {
        // Prefer embedded md5Manifest from JSON (ship .7z + .json only).
        if (!WriteFileUtf8(manifestDest, info.md5Manifest, error)) {
            return false;
        }
    } else {
        const std::wstring sidecarManifest = ReplaceArchiveExtension(archivePath, L".md5manifest");
        const std::wstring looseManifest = JoinPath(ParentOfPath(archivePath), info.manifest);
        if (FileExists(sidecarManifest)) {
            if (!CopyFileToWorkDir(sidecarManifest, manifestDest, error)) {
                return false;
            }
        } else if (loadedJsonFromSidecar && FileExists(looseManifest)) {
            if (!CopyFileToWorkDir(looseManifest, manifestDest, error)) {
                return false;
            }
        } else {
            if (sevenZipExe.empty() || !FileExists(sevenZipExe)) {
                error = L"7za.exe not available to extract pack manifest (no md5Manifest in JSON)";
                return false;
            }
            const std::vector<std::wstring> manifestMembers = {info.manifest};
            const ExtractResult manifestExtract = Extract7zArchiveSelective(
                sevenZipExe, archivePath, workDir, manifestMembers, {}, extractLogPath, false);
            if (!manifestExtract.success) {
                error = L"Could not extract pack manifest: " + FormatExtractFailureMessage(manifestExtract);
                return false;
            }
        }
    }

    return ValidateUpdatePackInfo(info, workDir, error);
}

bool ValidateUpdatePackInfo(const UpdatePackInfo& info, const std::wstring& workDir, std::wstring& error) {
    error.clear();
    const std::wstring manifestPath = JoinPath(workDir, info.manifest);
    if (!FileExists(manifestPath)) {
        error = L"Update pack manifest missing after extract: " + info.manifest;
        return false;
    }

    std::string actualMd5;
    std::wstring hashError;
    if (!ComputeFileMd5(manifestPath, actualMd5, hashError)) {
        error = L"Could not hash pack manifest: " + hashError;
        return false;
    }
    if (ToLowerAscii(actualMd5) != info.archiveMd5) {
        error = L"Update pack archiveMd5 does not match MedicatUpdate.md5 fingerprint";
        return false;
    }

    if (info.add.empty()) {
        error = L"Update pack add list is empty";
        return false;
    }

    std::map<std::wstring, bool> removeLookup;
    for (const std::wstring& path : info.remove) {
        removeLookup[ToLowerCopy(NormalizePackRelPath(path))] = true;
    }
    for (const std::wstring& path : info.add) {
        if (removeLookup.find(ToLowerCopy(NormalizePackRelPath(path))) != removeLookup.end()) {
            error = L"Update pack lists the same path in remove and add: " + path;
            return false;
        }
    }
    return true;
}

UpdatePackApplyResult ApplyUpdatePack(const std::wstring& sevenZipExe, const std::wstring& archivePath,
                                      const std::wstring& driveRoot, const std::wstring& workDir,
                                      ProgressCallback onProgress,
                                      const std::function<void(const std::wstring&)>& onLog,
                                      const std::wstring& extractLogPath) {
    UpdatePackApplyResult result;
    if (archivePath.empty()) {
        result.skipped = true;
        result.success = true;
        return result;
    }

    std::wstring error;
    {
        const std::wstring sidecarJson = ReplaceArchiveExtension(archivePath, L".json");
        if (onLog) {
            if (FileExists(sidecarJson)) {
                onLog(L"Reading update pack metadata from " + sidecarJson);
            } else {
                onLog(L"No sidecar .json beside pack; extracting info.json from archive (may take a while)...");
            }
        }
    }
    if (!LoadUpdatePackInfo(sevenZipExe, archivePath, workDir, result.info, error, extractLogPath)) {
        result.error = error;
        return result;
    }
    result.workDir = workDir;
    result.extractedManifestPath = JoinPath(workDir, result.info.manifest);

    if (onLog) {
        onLog(L"Applying update pack " + result.info.version + L" from " + archivePath);
    }

    std::wstring dest = driveRoot;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }

    for (const std::wstring& relative : result.info.remove) {
        if (result.info.bootablesOnly && IsBootablesProtectedPath(relative)) {
            if (onLog) {
                onLog(L"Update pack remove blocked (bootablesOnly): " + relative);
            }
            continue;
        }
        std::wstring rel = relative;
        for (wchar_t& ch : rel) {
            if (ch == L'/') {
                ch = L'\\';
            }
        }
        const std::wstring target = JoinPath(dest, rel);
        if (GetFileAttributesW(target.c_str()) == INVALID_FILE_ATTRIBUTES) {
            if (onLog) {
                onLog(L"Update pack remove skip (missing): " + relative);
            }
            // Still drop empty parents left behind by earlier partial removes.
            PruneEmptyParentDirs(target, dest, onLog);
            continue;
        }
        if (!DeletePathRecursive(target)) {
            result.error = L"Failed to remove path from drive: " + relative;
            return result;
        }
        if (onLog) {
            onLog(L"Update pack removed: " + relative);
        }
        PruneEmptyParentDirs(target, dest, onLog);
    }

    // Extract USB-relative add[] paths straight onto the drive (flat archive layout).
    std::vector<std::wstring> members;
    members.reserve(result.info.add.size());
    for (const std::wstring& relative : result.info.add) {
        if (result.info.bootablesOnly && IsBootablesProtectedPath(relative) &&
            !IsBootablesProtectedRootFile(relative)) {
            if (onLog) {
                onLog(L"Update pack add blocked (bootablesOnly): " + relative);
            }
            continue;
        }
        std::wstring member = NormalizePackRelPath(relative);
        for (wchar_t& ch : member) {
            if (ch == L'/') {
                ch = L'\\';
            }
        }
        members.push_back(member);
    }
    if (members.empty()) {
        result.error = L"Update pack has no extractable add paths";
        return result;
    }

    if (onLog) {
        onLog(L"Extracting update pack content to " + dest + L" (please wait while the archive opens)...");
    }

    const ExtractResult contentExtract = Extract7zArchiveSelective(
        sevenZipExe, archivePath, dest, members,
        [&](const ExtractProgress& progress) {
            if (onProgress) {
                onProgress(progress);
            }
        },
        extractLogPath, true);
    if (!contentExtract.success) {
        result.error = L"Failed extracting update pack content: " + FormatExtractFailureMessage(contentExtract);
        return result;
    }

    for (const std::wstring& relative : result.info.add) {
        if (result.info.bootablesOnly && IsBootablesProtectedPath(relative) &&
            !IsBootablesProtectedRootFile(relative)) {
            continue;
        }
        if (!PathExistsUnder(dest, relative)) {
            result.error = L"Update pack add path missing on drive after extract: " + relative;
            return result;
        }
    }

    if (onLog) {
        onLog(L"Update pack overlay complete (" + std::to_wstring(result.info.add.size()) + L" add, " +
              std::to_wstring(result.info.remove.size()) + L" remove)");
    }

    result.success = true;
    return result;
}

}  // namespace medicat
