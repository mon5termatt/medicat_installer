#pragma once

#include <string>

namespace medicat {

struct VentoyResult {
    bool success = false;
    int exitCode = -1;
    std::wstring error;
};

VentoyResult RunVentoyInstall(const std::wstring& ventoyExe, const std::wstring& driveLetter,
                              bool upgrade);
bool FormatDriveNtfs(const std::wstring& driveLetter, const std::wstring& label = L"Medicat");

}  // namespace medicat
