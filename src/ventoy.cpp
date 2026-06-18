#include "ventoy.h"

#include "drives.h"
#include "download.h"
#include "extract.h"
#include "offline.h"
#include "resource.h"
#include "util.h"

#include <windows.h>

#include <fstream>
#include <sstream>
#include <vector>

#include <cstring>

namespace medicat {

namespace {

constexpr UINT32 kVentoyPart1StartSector = 2048;
constexpr UINT32 kVentoyEfiPartSectors = (32u * 1024u * 1024u) / 512u;  // 32 MiB EFI partition

#pragma pack(push, 1)
struct VentoyPartTable {
    BYTE Active;
    BYTE StartHead;
    UINT16 StartSector : 6;
    UINT16 StartCylinder : 10;
    BYTE FsFlag;
    BYTE EndHead;
    UINT16 EndSector : 6;
    UINT16 EndCylinder : 10;
    UINT32 StartSectorId;
    UINT32 SectorCount;
};

struct VentoyMbrHead {
    BYTE BootCode[446];
    VentoyPartTable PartTbl[4];
    BYTE Byte55;
    BYTE ByteAA;
};

struct VentoyGptHdr {
    CHAR Signature[8];
    BYTE Version[4];
    UINT32 Length;
    UINT32 Crc;
    BYTE Reserved1[4];
    UINT64 EfiStartLBA;
    UINT64 EfiBackupLBA;
    UINT64 PartAreaStartLBA;
    UINT64 PartAreaEndLBA;
    GUID DiskGuid;
    UINT64 PartTblStartLBA;
    UINT32 PartTblTotNum;
    UINT32 PartTblEntryLen;
    UINT32 PartTblCrc;
    BYTE Reserved2[420];
};

struct VentoyGptPartTbl {
    GUID PartType;
    GUID PartGuid;
    UINT64 StartLBA;
    UINT64 LastLBA;
    UINT64 Attr;
    WCHAR Name[36];
};

struct VentoyGptInfo {
    VentoyMbrHead MBR;
    VentoyGptHdr Head;
    VentoyGptPartTbl PartTbl[128];
};
#pragma pack(pop)

// Partition layout check aligned with Ventoy2Disk IsVentoyPhyDrive (VTOYEFI + 32 MiB part 2).
bool IsVentoyPhysicalDrive(const DWORD phyDrive) {
    std::wostringstream path;
    path << L"\\\\.\\PhysicalDrive" << phyDrive;
    const HANDLE disk =
        CreateFileW(path.str().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
                    nullptr);
    if (disk == INVALID_HANDLE_VALUE) {
        return false;
    }

    VentoyMbrHead mbr{};
    DWORD read = 0;
    if (!ReadFile(disk, &mbr, sizeof(mbr), &read, nullptr) || read != sizeof(mbr)) {
        CloseHandle(disk);
        return false;
    }

    if (mbr.Byte55 != 0x55 || mbr.ByteAA != 0xAA) {
        CloseHandle(disk);
        return false;
    }

    if (mbr.PartTbl[0].FsFlag == 0xEE) {
        VentoyGptInfo gpt{};
        SetFilePointer(disk, 0, nullptr, FILE_BEGIN);
        const BOOL gptRead = ReadFile(disk, &gpt, sizeof(gpt), &read, nullptr);
        CloseHandle(disk);
        if (!gptRead || read != sizeof(gpt)) {
            return false;
        }

        if (memcmp(gpt.Head.Signature, "EFI PART", 8) != 0) {
            return false;
        }

        static const WCHAR kVtoyEfiName[] = L"VTOYEFI";
        if (memcmp(gpt.PartTbl[1].Name, kVtoyEfiName, sizeof(kVtoyEfiName) - sizeof(WCHAR)) != 0) {
            return false;
        }

        if (gpt.PartTbl[0].StartLBA != kVentoyPart1StartSector) {
            return false;
        }

        const UINT32 part2SectorCount =
            static_cast<UINT32>(gpt.PartTbl[1].LastLBA + 1 - gpt.PartTbl[1].StartLBA);
        if (gpt.PartTbl[1].StartLBA != gpt.PartTbl[0].LastLBA + 1 ||
            part2SectorCount != kVentoyEfiPartSectors) {
            return false;
        }

        return true;
    }

    CloseHandle(disk);

    if (mbr.PartTbl[0].StartSectorId != kVentoyPart1StartSector) {
        return false;
    }

    const UINT32 part2Start = mbr.PartTbl[0].StartSectorId + mbr.PartTbl[0].SectorCount;
    if (mbr.PartTbl[1].StartSectorId != part2Start || mbr.PartTbl[1].SectorCount != kVentoyEfiPartSectors) {
        return false;
    }

    return true;
}

}  // namespace

namespace {

void LogLine(const VentoyEnsureOptions& options, const std::wstring& message) {
    if (options.onLog) {
        options.onLog(message);
    }
}

void SetStatus(const VentoyEnsureOptions& options, const std::wstring& message) {
    if (options.onStatus) {
        options.onStatus(message);
    }
}

int RunHiddenProcess(const std::wstring& commandLine, const std::wstring& workingDir = L"") {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> cmd(commandLine.begin(), commandLine.end());
    cmd.push_back(L'\0');

    PROCESS_INFORMATION pi{};
    const wchar_t* work = workingDir.empty() ? nullptr : workingDir.c_str();
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, work,
                        &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(code);
}

bool RemoveDirectoryTree(const std::wstring& path) {
    if (path.empty() || path.size() < 3) {
        return false;
    }
    std::wstring pattern = JoinPath(path, L"*");
    WIN32_FIND_DATAW fd{};
    const HANDLE find = FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return RemoveDirectoryW(path.c_str()) != FALSE;
    }

    do {
        const std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        const std::wstring full = JoinPath(path, name);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            RemoveDirectoryTree(full);
        } else {
            SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(full.c_str());
        }
    } while (FindNextFileW(find, &fd));
    FindClose(find);
    return RemoveDirectoryW(path.c_str()) != FALSE;
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

std::wstring NormalizeVersion(std::wstring version) {
    while (!version.empty() && (version.front() == L'v' || version.front() == L'V')) {
        version.erase(version.begin());
    }
    return version;
}

std::wstring ParseLatestTagFromGitHubJson(const std::wstring& json) {
    const std::wstring needle = L"\"tag_name\":\"";
    const size_t pos = json.find(needle);
    if (pos == std::wstring::npos) {
        return L"";
    }

    const size_t start = pos + needle.size();
    const size_t end = json.find(L'"', start);
    if (end == std::wstring::npos || end <= start) {
        return L"";
    }
    return json.substr(start, end - start);
}

std::vector<std::wstring> ParseReleaseTagsFromGitHubJson(const std::wstring& json) {
    std::vector<std::wstring> tags;
    const std::wstring needle = L"\"tag_name\":\"";
    size_t pos = 0;
    while ((pos = json.find(needle, pos)) != std::wstring::npos) {
        const size_t start = pos + needle.size();
        const size_t end = json.find(L'"', start);
        if (end != std::wstring::npos && end > start) {
            const std::wstring version = NormalizeVersion(json.substr(start, end - start));
            if (!version.empty()) {
                tags.push_back(version);
            }
        }
        pos = start;
    }
    return tags;
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

}  // namespace

VentoyResult FetchLatestVentoyVersion(std::wstring& version) {
    VentoyResult result;
    std::wstring body;
    std::wstring error;
    if (!HttpGet(L"https://api.github.com/repos/ventoy/Ventoy/releases/latest", body, error)) {
        if (ResolveOfflineVentoyVersion(L"", version)) {
            result.success = true;
            result.version = version;
            return result;
        }
        result.error = L"Could not fetch Ventoy version: " + error;
        return result;
    }

    version = NormalizeVersion(ParseLatestTagFromGitHubJson(body));
    if (version.empty()) {
        if (ResolveOfflineVentoyVersion(L"", version)) {
            result.success = true;
            result.version = version;
            return result;
        }
        result.error = L"Could not parse latest Ventoy version from GitHub";
        return result;
    }

    result.success = true;
    result.version = version;
    return result;
}

VentoyResult FetchVentoyVersions(std::vector<std::wstring>& versions) {
    VentoyResult result;
    versions.clear();

    for (int page = 1; page <= 20; ++page) {
        std::wstring body;
        std::wstring error;
        const std::wstring url = L"https://api.github.com/repos/ventoy/Ventoy/releases?per_page=100&page=" +
                                 std::to_wstring(page);
        if (!HttpGet(url, body, error)) {
            if (versions.empty()) {
                result.error = L"Could not fetch Ventoy versions: " + error;
            }
            break;
        }

        const std::vector<std::wstring> pageVersions = ParseReleaseTagsFromGitHubJson(body);
        if (pageVersions.empty()) {
            break;
        }

        versions.insert(versions.end(), pageVersions.begin(), pageVersions.end());
        if (pageVersions.size() < 100) {
            break;
        }
    }

    if (versions.empty()) {
        if (result.error.empty()) {
            result.error = L"Could not parse Ventoy versions from GitHub";
        }
        return result;
    }

    result.success = true;
    return result;
}

bool LoadBundledVentoyVersionList(const HINSTANCE instance, std::vector<std::wstring>& versions) {
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(IDR_VENTOY_VERSIONS), RT_RCDATA);
    if (!resource) {
        return false;
    }

    const HGLOBAL loaded = LoadResource(instance, resource);
    if (!loaded) {
        return false;
    }

    const DWORD size = SizeofResource(instance, resource);
    const void* data = LockResource(loaded);
    if (!data || size == 0) {
        return false;
    }

    const int len = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(data), static_cast<int>(size),
                                        nullptr, 0);
    if (len <= 0) {
        return false;
    }

    std::wstring content(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(data), static_cast<int>(size), content.data(), len);
    return ParseVentoyVersionText(content, versions);
}

VentoyResult EnsureVentoyReady(const VentoyEnsureOptions& options) {
    VentoyResult result;

    std::wstring targetVersion = NormalizeVersion(options.pinVersion);
    if (targetVersion.empty()) {
        SetStatus(options, L"checking_ventoy");
        const VentoyResult latest = FetchLatestVentoyVersion(targetVersion);
        if (!latest.success) {
            return latest;
        }
        LogLine(options, L"Latest Ventoy version: v" + targetVersion);
    } else {
        LogLine(options, L"Using pinned Ventoy version: v" + targetVersion);
    }

    const std::wstring ventoyDir = JoinPath(options.root, L"Ventoy2Disk");
    const std::wstring ventoyExe = JoinPath(ventoyDir, L"Ventoy2Disk.exe");
    const std::wstring versionFile = JoinPath(ventoyDir, L"ventoy\\version");
    const std::wstring localVersion = NormalizeVersion(ReadTextFile(versionFile));

    const bool needsDownload =
        !FileExists(ventoyExe) || localVersion.empty() || localVersion != targetVersion;

    if (needsDownload) {
        if (!localVersion.empty() && localVersion != targetVersion) {
            LogLine(options, L"Updating Ventoy from v" + localVersion + L" to v" + targetVersion);
        } else if (!OfflineVentoyZipExists(targetVersion)) {
            LogLine(options, L"Downloading Ventoy v" + targetVersion);
        }

        const std::wstring cachedZip = GetOfflineVentoyZipPath(targetVersion);
        const std::wstring workingZip = JoinPath(options.root, L"ventoy.zip");
        std::wstring zipPath;
        bool deleteWorkingZip = false;

        if (OfflineVentoyZipExists(targetVersion)) {
            LogLine(options, L"Using offline Ventoy cache for v" + targetVersion);
            zipPath = cachedZip;
        } else {
            SetStatus(options, L"downloading_ventoy:" + targetVersion);

            const std::wstring zipUrl = L"https://github.com/ventoy/Ventoy/releases/download/v" + targetVersion +
                                        L"/ventoy-" + targetVersion + L"-windows.zip";
            zipPath = workingZip;
            deleteWorkingZip = true;

            std::wstring error;
            if (!HttpDownloadFile(zipUrl, zipPath, error)) {
                result.error = L"Failed to download Ventoy: " + error;
                return result;
            }

            CacheVentoyZip(targetVersion, zipPath);
        }

        SetStatus(options, L"extracting_ventoy");
        LogLine(options, L"Extracting Ventoy archive");

        const ExtractResult extract = Extract7zArchive(
            options.sevenZipExe, zipPath, options.root, 0, 0,
            [&](const ExtractProgress& progress) {
                if (progress.percent >= 0) {
                    SetStatus(options, L"extracting_ventoy:" + std::to_wstring(progress.percent));
                }
            });

        if (deleteWorkingZip) {
            DeleteFileW(zipPath.c_str());
        }

        if (!extract.success) {
            result.error = L"Failed to extract Ventoy: " + extract.error;
            return result;
        }

        const std::wstring extractedDir = JoinPath(options.root, L"ventoy-" + targetVersion);
        if (!FileExists(JoinPath(extractedDir, L"Ventoy2Disk.exe"))) {
            result.error = L"Ventoy archive did not contain Ventoy2Disk.exe";
            return result;
        }

        if (FileExists(ventoyDir)) {
            RemoveDirectoryTree(ventoyDir);
        }

        if (!MoveFileW(extractedDir.c_str(), ventoyDir.c_str())) {
            result.error = L"Failed to rename extracted Ventoy folder";
            return result;
        }

        LogLine(options, L"Ventoy v" + targetVersion + L" ready");
    } else {
        LogLine(options, L"Local Ventoy v" + localVersion + L" is up to date");
    }

    if (!FileExists(ventoyExe)) {
        result.error = L"Ventoy2Disk.exe not found after download";
        return result;
    }

    result.success = true;
    result.ventoyExe = ventoyExe;
    result.version = targetVersion;
    return result;
}

VentoyDetectionResult DetectVentoyOnDrive(const std::wstring& driveLetter) {
    VentoyDetectionResult result;
    auto log = [&](const std::wstring& line) { result.logLines.push_back(line); };

    if (driveLetter.size() < 2) {
        log(L"Ventoy detection: invalid drive letter");
        return result;
    }

    log(L"Ventoy detection on " + driveLetter + L":");

    const DriveIdentity identity = GetDriveIdentity(driveLetter);
    bool layoutMatched = false;
    if (!identity.valid) {
        log(L"  physical disk: could not resolve drive identity");
    } else {
        std::wstring diskList;
        for (const DWORD diskNumber : identity.diskNumbers) {
            if (!diskList.empty()) {
                diskList += L", ";
            }
            diskList += L"PhysicalDrive" + std::to_wstring(diskNumber);
        }
        log(L"  physical disk(s): " + diskList);

        for (const DWORD diskNumber : identity.diskNumbers) {
            const bool layoutMatch = IsVentoyPhysicalDrive(diskNumber);
            log(L"  VTOYEFI layout on PhysicalDrive" + std::to_wstring(diskNumber) + L": " +
                (layoutMatch ? L"matched (Ventoy2Disk partition layout)" : L"not matched"));
            if (layoutMatch) {
                layoutMatched = true;
            }
        }
    }

    result.installed = layoutMatched;

    if (result.installed) {
        log(L"  result: Ventoy found (VTOYEFI partition layout)");
    } else {
        log(L"  result: Ventoy not found");
    }

    return result;
}

bool TestVentoyInstalled(const std::wstring& driveLetter) {
    return DetectVentoyOnDrive(driveLetter).installed;
}

VentoyResult RunVentoyInstall(const std::wstring& ventoyExe, const std::wstring& driveLetter,
                              const bool upgrade, const VentoyInstallOptions& options) {
    VentoyResult result;
    if (!FileExists(ventoyExe)) {
        result.error = L"Ventoy2Disk.exe not found";
        return result;
    }

    std::wstring drive = driveLetter;
    if (drive.size() >= 2 && drive[1] == L':') {
        drive = drive.substr(0, 2);
    }

    const std::wstring ventoyWork = GetExeDirectory();
    const std::wstring ventoyDir = JoinPath(ventoyWork, L"Ventoy2Disk");
    std::wstring args =
        upgrade ? (L"VTOYCLI /U /Drive:" + drive) : (L"VTOYCLI /I /Drive:" + drive + L" /NOUSBCheck");
    if (options.useGpt) {
        args += L" /GPT";
    }
    if (!options.enableSecureBoot) {
        args += L" /NOSB";
    }

    std::wstring cmd = L"\"" + ventoyExe + L"\" " + args;
    result.exitCode = RunHiddenProcess(cmd, ventoyDir);
    result.success = (result.exitCode == 0);
    result.ventoyExe = ventoyExe;
    if (!result.success) {
        std::wostringstream err;
        err << L"Ventoy failed with exit code " << result.exitCode;
        result.error = err.str();
    }
    return result;
}

bool FormatDriveNtfs(const std::wstring& driveLetter, const std::wstring& label) {
    if (driveLetter.empty()) {
        return false;
    }
    wchar_t letter = driveLetter[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    if (letter < L'A' || letter > L'Z') {
        return false;
    }
    const std::wstring drive = std::wstring(1, letter) + L":";
    const std::wstring cmd = L"format.com " + drive + L" /FS:NTFS /X /Q /V:" + label + L" /Y";
    return RunHiddenProcess(cmd) == 0;
}

}  // namespace medicat
