#include "drives.h"

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

bool IsFileBackedVirtualDisk(const DWORD diskNumber) {
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
    return desc->BusType == BusTypeFileBackedVirtual;
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

    // Skip tiny volumes (matches PS installer: Size > 1GB)
    if (totalBytes.QuadPart <= 1024ULL * 1024ULL * 1024ULL) {
        return false;
    }

    DriveInfo info;
    info.letter = std::wstring(1, letter) + L":";
    info.label = QueryVolumeLabel(root);
    info.kind = kind;
    info.totalBytes = totalBytes.QuadPart;
    info.freeBytes = freeBytesAvailable.QuadPart;

    const double freeGb = static_cast<double>(info.freeBytes) / (1024.0 * 1024.0 * 1024.0);
    const double totalGb = static_cast<double>(info.totalBytes) / (1024.0 * 1024.0 * 1024.0);

    std::wostringstream display;
    display << info.letter << L' ' << kind << L" - " << freeGb << L"GB free of " << totalGb << L"GB";
    if (!info.label.empty()) {
        display << L" (" << info.label << L')';
    }
    info.display = display.str();
    drives.push_back(std::move(info));
    return true;
}

}  // namespace

std::vector<DriveInfo> ListTargetDrives() {
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
        const bool isUsb = driveType == DRIVE_REMOVABLE;

        if (!isVhd && !isUsb) {
            continue;
        }

        TryAddDrive(drives, letter, isVhd ? L"(VHD)" : L"(USB)");
    }

    return drives;
}

int DefaultDriveIndex(const std::vector<DriveInfo>& drives) {
    for (size_t i = 0; i < drives.size(); ++i) {
        if (drives[i].kind == L"(VHD)") {
            return static_cast<int>(i);
        }
    }
    return drives.empty() ? -1 : 0;
}

uint64_t GetDriveFreeBytes(const std::wstring& root) {
    std::wstring normalized = root;
    if (normalized.size() == 2 && normalized[1] == L':') {
        normalized += L'\\';
    }
    return QueryFreeBytes(normalized).QuadPart;
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
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

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
