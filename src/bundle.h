#pragma once

#include "resource.h"

#include <windows.h>

#include <string>

namespace medicat {

struct BundledTools {
    std::wstring dir;
    std::wstring sevenZa;
    std::wstring aria2c;
    std::wstring md5Manifest;
    bool ok = false;
    std::wstring error;
};

// Extract embedded 7za.exe (arch-selected), aria2c.exe, and MedicatFiles.md5.gz to %TEMP%\MedicatInstaller\{pid}\.
BundledTools EnsureBundledTools(HINSTANCE instance);

// Write the installer icon (res/icon.ico) to <driveRoot>\autorun.ico.
bool WriteInstallerAutorunIcon(HINSTANCE instance, const std::wstring& driveRoot);

// If <driveRoot>\autorun.ico is missing or empty, write it from the installer icon.
bool EnsureInstallerAutorunIcon(HINSTANCE instance, const std::wstring& driveRoot);

}  // namespace medicat
