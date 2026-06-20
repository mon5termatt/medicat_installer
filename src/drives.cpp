#include "drives.h"

#include "cancel.h"
#include "i18n.h"
#include "util.h"

#include <windows.h>
#include <winioctl.h>

#include <set>
#include <sstream>
#include <vector>

namespace medicat {

namespace {

std::wstring QueryVolumeLabel(const std::wstring& root) {
    wchar_t label[MAX_PATH + 1]{};
    wchar_t fs[MAX_PATH + 1]{};
    DWORD serial = 0;
    DWORD maxComp = 0;
    DWORD flags = 0;
    if (!GetVolumeInformationW(root.c_str(), label, MAX_PATH, &serial, &maxComp, &flags, fs, MAX_PATH)) {
        return L"";
    }
    return label;
}

ULARGE_INTEGER QueryFreeBytes(const std::wstring& root) {
    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        return freeBytesAvailable;
    }
    ULARGE_INTEGER zero{};
    return zero;
}

bool QueryDiskBusType(const DWORD diskNumber, STORAGE_BUS_TYPE& busType) {
    busType = BusTypeUnknown;
    std::wostringstream path;
    path << L"\\\\.\\PhysicalDrive" << diskNumber;
    const HANDLE disk =
        CreateFileW(path.str().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (disk == INVALID_HANDLE_VALUE) {
        return false;
    }

    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    BYTE buffer[1024]{};
    DWORD returned = 0;
    const BOOL ok = DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer,
                                    sizeof(buffer), &returned, nullptr);
    CloseHandle(disk);
    if (!ok || returned < sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
        return false;
    }

    const auto* desc = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(buffer);
    busType = desc->BusType;
    return true;
}

bool IsFileBackedVirtualDisk(const DWORD diskNumber) {
    STORAGE_BUS_TYPE busType = BusTypeUnknown;
    return QueryDiskBusType(diskNumber, busType) && busType == BusTypeFileBackedVirtual;
}

bool IsUsbBusDisk(const DWORD diskNumber) {
    STORAGE_BUS_TYPE busType = BusTypeUnknown;
    if (!QueryDiskBusType(diskNumber, busType)) {
        return false;
    }
    return busType == BusTypeUsb || busType == BusTypeSd || busType == BusTypeMmc;
}

std::set<wchar_t> GetVhdDriveLetters() {
    std::set<wchar_t> letters;
    const DWORD mask = GetLogicalDrives();

    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (letter == L'C') {
            continue;
        }
        const int bit = letter - L'A';
        if ((mask & (1u << bit)) == 0) {
            continue;
        }

        wchar_t volumePath[] = L"\\\\.\\?:";
        volumePath[4] = letter;
        const HANDLE volume =
            CreateFileW(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (volume == INVALID_HANDLE_VALUE) {
            continue;
        }

        std::vector<BYTE> extentsBuf(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 8);
        DWORD returned = 0;
        if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                             static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
            if (GetLastError() != ERROR_MORE_DATA) {
                CloseHandle(volume);
                continue;
            }
            extentsBuf.resize(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 32);
            returned = 0;
            if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                                 static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
                CloseHandle(volume);
                continue;
            }
        }
        CloseHandle(volume);

        if (returned < sizeof(VOLUME_DISK_EXTENTS)) {
            continue;
        }

        const auto* extents = reinterpret_cast<const VOLUME_DISK_EXTENTS*>(extentsBuf.data());
        for (DWORD i = 0; i < extents->NumberOfDiskExtents; ++i) {
            if (IsFileBackedVirtualDisk(extents->Extents[i].DiskNumber)) {
                letters.insert(letter);
                break;
            }
        }
    }

    return letters;
}

bool TryAddDrive(std::vector<DriveInfo>& drives, wchar_t letter, const std::wstring& kind) {
    const std::wstring root = std::wstring(1, letter) + L":\\";

    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (!GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        return false;
    }

    // MediCat requires at least 30 GiB total capacity (leeway below nominal 32 GB drives).
    if (totalBytes.QuadPart < kMinDriveCapacityBytes) {
        return false;
    }

    DriveInfo info;
    info.letter = std::wstring(1, letter) + L":";
    info.label = QueryVolumeLabel(root);
    info.kind = kind;
    info.totalBytes = totalBytes.QuadPart;
    info.freeBytes = freeBytesAvailable.QuadPart;

    const auto freeGb = static_cast<uint64_t>(info.freeBytes / (1024ULL * 1024ULL * 1024ULL));
    const auto totalGb = static_cast<uint64_t>(info.totalBytes / (1024ULL * 1024ULL * 1024ULL));

    std::wstring typeLabel;
    if (kind == L"VHD") {
        typeLabel = i18n::Tr(L"ui.drive_type_vhd");
    } else if (kind == L"HDD") {
        typeLabel = i18n::Tr(L"ui.drive_type_hdd");
    } else {
        typeLabel = i18n::Tr(L"ui.drive_type_usb");
    }

    info.kind = typeLabel;

    std::wstring driveName = info.letter;
    if (!info.label.empty()) {
        std::wstring safeLabel = info.label;
        for (wchar_t& ch : safeLabel) {
            if (ch == L'"') {
                ch = L'\'';
            }
        }
        driveName += L" \"" + safeLabel + L"\"";
    }

    info.display = i18n::Tr(L"ui.drive_format", driveName, typeLabel,
                            std::to_wstring(freeGb), std::to_wstring(totalGb));
    drives.push_back(std::move(info));
    return true;
}

wchar_t NormalizeDriveLetter(const std::wstring& driveLetter) {
    if (driveLetter.empty()) {
        return L'\0';
    }
    wchar_t letter = driveLetter[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    return letter;
}

bool GetVolumeDiskNumbers(const wchar_t letter, std::vector<DWORD>& diskNumbers) {
    diskNumbers.clear();
    if (letter < L'A' || letter > L'Z') {
        return false;
    }

    wchar_t volumePath[] = L"\\\\.\\?:";
    volumePath[4] = letter;
    const HANDLE volume =
        CreateFileW(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (volume == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::vector<BYTE> extentsBuf(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 8);
    DWORD returned = 0;
    if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                         static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
        if (GetLastError() != ERROR_MORE_DATA) {
            CloseHandle(volume);
            return false;
        }
        extentsBuf.resize(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 32);
        returned = 0;
        if (!DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, extentsBuf.data(),
                             static_cast<DWORD>(extentsBuf.size()), &returned, nullptr)) {
            CloseHandle(volume);
            return false;
        }
    }
    CloseHandle(volume);

    if (returned < sizeof(VOLUME_DISK_EXTENTS)) {
        return false;
    }

    const auto* extents = reinterpret_cast<const VOLUME_DISK_EXTENTS*>(extentsBuf.data());
    diskNumbers.reserve(extents->NumberOfDiskExtents);
    for (DWORD i = 0; i < extents->NumberOfDiskExtents; ++i) {
        diskNumbers.push_back(extents->Extents[i].DiskNumber);
    }
    return !diskNumbers.empty();
}

bool IsUsbBusVolume(const wchar_t letter) {
    std::vector<DWORD> diskNumbers;
    if (!GetVolumeDiskNumbers(letter, diskNumbers)) {
        return false;
    }
    for (const DWORD diskNumber : diskNumbers) {
        if (IsUsbBusDisk(diskNumber)) {
            return true;
        }
    }
    return false;
}

bool IdentityMatches(const DriveIdentity& a, const DriveIdentity& b) {
    if (!a.valid || !b.valid) {
        return false;
    }
    for (const DWORD diskA : a.diskNumbers) {
        for (const DWORD diskB : b.diskNumbers) {
            if (diskA == diskB) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

std::vector<DriveInfo> ListTargetDrives(const bool includeAllDrives) {
    std::vector<DriveInfo> drives;
    const std::set<wchar_t> vhdLetters = GetVhdDriveLetters();
    const DWORD mask = GetLogicalDrives();

    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (letter == L'C') {
            continue;
        }
        const int bit = letter - L'A';
        if ((mask & (1u << bit)) == 0) {
            continue;
        }

        const std::wstring root = std::wstring(1, letter) + L":\\";
        const UINT driveType = GetDriveTypeW(root.c_str());
        const bool isVhd = vhdLetters.count(letter) > 0;
        const bool isUsbBus = IsUsbBusVolume(letter);
        const bool isUsb = driveType == DRIVE_REMOVABLE || isUsbBus;
        const bool isFixed = driveType == DRIVE_FIXED && !isUsbBus;

        if (isVhd) {
            TryAddDrive(drives, letter, L"VHD");
        } else if (isUsb) {
            TryAddDrive(drives, letter, L"USB");
        } else if (includeAllDrives && isFixed) {
            TryAddDrive(drives, letter, L"HDD");
        }
    }

    return drives;
}

int DefaultDriveIndex(const std::vector<DriveInfo>& drives) {
    for (size_t i = 0; i < drives.size(); ++i) {
        if (drives[i].kind == i18n::Tr(L"ui.drive_type_vhd")) {
            return static_cast<int>(i);
        }
    }
    return drives.empty() ? -1 : 0;
}

DriveIdentity GetDriveIdentity(const std::wstring& driveLetter) {
    DriveIdentity identity;
    const wchar_t letter = NormalizeDriveLetter(driveLetter);
    if (letter == L'\0') {
        return identity;
    }

    const std::wstring root = std::wstring(1, letter) + L":\\";
    DWORD serial = 0;
    if (GetVolumeInformationW(root.c_str(), nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
        identity.volumeSerial = serial;
    }

    identity.valid = GetVolumeDiskNumbers(letter, identity.diskNumbers);
    return identity;
}

namespace {

bool DriveLetterMatchesIdentity(const std::wstring& driveLetter, const DriveIdentity& identity) {
    const wchar_t letter = NormalizeDriveLetter(driveLetter);
    if (letter == L'\0') {
        return false;
    }

    const std::wstring root = std::wstring(1, letter) + L":\\";
    if (GetDriveTypeW(root.c_str()) == DRIVE_NO_ROOT_DIR) {
        return false;
    }

    return IdentityMatches(identity, GetDriveIdentity(driveLetter));
}

std::wstring FindDriveLetterForIdentity(const DriveIdentity& identity) {
    const DWORD mask = GetLogicalDrives();
    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (letter == L'C') {
            continue;
        }
        const int bit = letter - L'A';
        if ((mask & (1u << bit)) == 0) {
            continue;
        }

        const std::wstring candidate = std::wstring(1, letter) + L":";
        if (DriveLetterMatchesIdentity(candidate, identity)) {
            return candidate;
        }
    }
    return L"";
}

}  // namespace

std::wstring ResolveDriveLetterAfterVentoy(const std::wstring& expectedLetter, const DriveIdentity& before) {
    if (!before.valid) {
        return expectedLetter;
    }

    for (int attempt = 0; attempt < 20; ++attempt) {
        if (attempt > 0) {
            Sleep(500);
        }

        if (DriveLetterMatchesIdentity(expectedLetter, before)) {
            return expectedLetter;
        }

        const std::wstring found = FindDriveLetterForIdentity(before);
        if (!found.empty()) {
            return found;
        }
    }

    return L"";
}

uint64_t GetDriveTotalBytes(const std::wstring& driveLetter) {
    const wchar_t letter = NormalizeDriveLetter(driveLetter);
    if (letter == L'\0') {
        return 0;
    }

    const std::wstring root = std::wstring(1, letter) + L":\\";
    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (!GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        return 0;
    }
    return totalBytes.QuadPart;
}

bool MeetsMinimumDriveCapacity(const uint64_t totalBytes) {
    return totalBytes >= kMinDriveCapacityBytes;
}

bool MeetsMinimumDriveCapacity(const std::wstring& driveLetter) {
    return MeetsMinimumDriveCapacity(GetDriveTotalBytes(driveLetter));
}

uint64_t GetDriveFreeBytes(const std::wstring& root) {
    std::wstring normalized = root;
    if (normalized.size() == 2 && normalized[1] == L':') {
        normalized += L'\\';
    }
    return QueryFreeBytes(normalized).QuadPart;
}

bool IsDriveLetterPresent(const std::wstring& driveLetter) {
    if (driveLetter.size() < 2 || driveLetter[1] != L':') {
        return false;
    }

    wchar_t letter = driveLetter[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    if (letter < L'A' || letter > L'Z') {
        return false;
    }

    const int bit = letter - L'A';
    const DWORD mask = GetLogicalDrives();
    return (mask & (1u << bit)) != 0;
}

std::wstring FormatWindowsError(const DWORD error) {
    if (error == 0) {
        return L"Unknown error";
    }

    wchar_t* message = nullptr;
    const DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                         FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr, error, 0, reinterpret_cast<LPWSTR>(&message), 0, nullptr);
    std::wstring out;
    if (len != 0 && message) {
        out.assign(message, len);
        while (!out.empty() && (out.back() == L'\r' || out.back() == L'\n')) {
            out.pop_back();
        }
    } else {
        out = L"Windows error " + std::to_wstring(error);
    }
    if (message) {
        LocalFree(message);
    }
    return out;
}

DestinationDriveStatus CheckDestinationDrive(const std::wstring& root, std::wstring* errorDetail) {
    if (root.size() < 2 || root[1] != L':') {
        return DestinationDriveStatus::Ok;
    }

    wchar_t letter = root[0];
    if (letter >= L'a' && letter <= L'z') {
        letter = static_cast<wchar_t>(letter - L'a' + L'A');
    }
    const std::wstring driveLetter = std::wstring(1, letter) + L":";
    if (!IsDriveLetterPresent(driveLetter)) {
        return DestinationDriveStatus::Removed;
    }

    std::wstring normalized = root;
    if (normalized.size() == 2 && normalized[1] == L':') {
        normalized += L'\\';
    }

    if (!GetVolumeInformationW(normalized.c_str(), nullptr, 0, nullptr, nullptr, nullptr, nullptr, 0)) {
        if (errorDetail) {
            *errorDetail = FormatWindowsError(GetLastError());
        }
        return DestinationDriveStatus::IoError;
    }

    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFree{};
    if (!GetDiskFreeSpaceExW(normalized.c_str(), &freeBytesAvailable, &totalBytes, &totalFree)) {
        if (errorDetail) {
            *errorDetail = FormatWindowsError(GetLastError());
        }
        return DestinationDriveStatus::IoError;
    }

    const HANDLE rootDir =
        CreateFileW(normalized.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (rootDir == INVALID_HANDLE_VALUE) {
        if (errorDetail) {
            *errorDetail = FormatWindowsError(GetLastError());
        }
        return DestinationDriveStatus::IoError;
    }
    CloseHandle(rootDir);

    return DestinationDriveStatus::Ok;
}

uint64_t GetArchiveUncompressedSize(const std::wstring& sevenZipExe, const std::wstring& archivePath) {
    const std::wstring listOut = JoinPath(GetExeDirectory(), L"_7za_list_tmp.txt");
    DeleteFileW(listOut.c_str());

    std::wstring cmd =
        L"cmd /c \"\"" + sevenZipExe + L"\"\" l -slt \"\"" + archivePath + L"\"\" > \"\"" + listOut + L"\"\" 2>&1\"";
    std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
    cmdLine.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        return 0;
    }

    ChildProcessRegistration childProcess(pi.hProcess);
    while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT) {
        if (IsCancelRequested()) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (IsCancelRequested()) {
        DeleteFileW(listOut.c_str());
        return 0;
    }

    HANDLE h = CreateFileW(listOut.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return 0;
    }
    std::string utf8;
    char chunk[4096];
    DWORD read = 0;
    while (ReadFile(h, chunk, sizeof(chunk), &read, nullptr) && read > 0) {
        utf8.append(chunk, chunk + read);
    }
    CloseHandle(h);
    DeleteFileW(listOut.c_str());

    const int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(static_cast<size_t>(wideLen > 0 ? wideLen - 1 : 0), L'\0');
    if (wideLen > 0) {
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), wideLen);
    }

    bool inFile = false;
    uint64_t size = 0;
    uint64_t total = 0;
    for (const auto& line : SplitLines(wide)) {
        if (line == L"----------") {
            if (inFile && size > 0) {
                total += size;
            }
            inFile = true;
            size = 0;
            continue;
        }
        if (line.rfind(L"Path = ", 0) == 0) {
            const std::wstring path = line.substr(7);
            if (path.empty() || path.back() == L'\\' || path.back() == L'/') {
                inFile = false;
                size = 0;
            }
            continue;
        }
        if (inFile && line.rfind(L"Size = ", 0) == 0) {
            size = _wcstoui64(line.c_str() + 7, nullptr, 10);
        }
    }
    if (inFile && size > 0) {
        total += size;
    }
    return total;
}

}  // namespace medicat
