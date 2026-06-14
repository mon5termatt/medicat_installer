#pragma once

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

// USB + mounted VHD/VHDX drives (excludes C:). Matches legacy installer behavior.
std::vector<DriveInfo> ListTargetDrives();
int DefaultDriveIndex(const std::vector<DriveInfo>& drives);
uint64_t GetDriveFreeBytes(const std::wstring& root);
uint64_t GetArchiveUncompressedSize(const std::wstring& sevenZipExe, const std::wstring& archivePath);

}  // namespace medicat
