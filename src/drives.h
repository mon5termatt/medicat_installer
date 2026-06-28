#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace medicat {

struct DriveInfo {
    std::wstring letter;   // e.g. L"E:"
    std::wstring label;
    std::wstring kind;     // L"USB", L"VHD", L"HDD"
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    std::wstring display;
};

struct DriveIdentity {
    DWORD volumeSerial = 0;
    std::vector<DWORD> diskNumbers;
    bool valid = false;
};

// Minimum total drive capacity (28 GiB — nominal "32 GB" sticks report ~29.8 GiB).
constexpr uint64_t kMinDriveCapacityBytes = 28ULL * 1024ULL * 1024ULL * 1024ULL;

// USB + mounted VHD/VHDX drives (excludes C:). Set includeAllDrives for fixed disks too.
std::vector<DriveInfo> ListTargetDrives(bool includeAllDrives = false);
int DefaultDriveIndex(const std::vector<DriveInfo>& drives);
DriveIdentity GetDriveIdentity(const std::wstring& driveLetter);
std::wstring ResolveDriveLetterAfterVentoy(const std::wstring& expectedLetter, const DriveIdentity& before);
uint64_t GetDriveTotalBytes(const std::wstring& driveLetter);
bool MeetsMinimumDriveCapacity(const std::wstring& driveLetter);
bool MeetsMinimumDriveCapacity(uint64_t totalBytes);
uint64_t GetDriveFreeBytes(const std::wstring& root);
uint64_t GetArchiveUncompressedSize(const std::wstring& sevenZipExe, const std::wstring& archivePath);

// True when the drive letter is still assigned (GetLogicalDrives bit set).
bool IsDriveLetterPresent(const std::wstring& driveLetter);

enum class DestinationDriveStatus {
    Ok,
    Removed,
    IoError,
};

// Checks that a destination root (e.g. E:\) is still mounted and readable.
DestinationDriveStatus CheckDestinationDrive(const std::wstring& root, std::wstring* errorDetail = nullptr);

}  // namespace medicat
