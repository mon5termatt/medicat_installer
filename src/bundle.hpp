#pragma once

#include "resource.hpp"

#include <windows.h>

#include <string>

namespace medicat {

struct BundledTools {
    std::wstring dir;
    std::wstring sevenZa;
    std::wstring sevenZ;
    std::wstring md5Manifest;
    bool ok = false;
    std::wstring error;
};

// Extract embedded 7za.exe, 7z.exe, and MedicatFiles.md5 to %TEMP%\MedicatInstaller\{pid}\ (cached by size).
BundledTools EnsureBundledTools(HINSTANCE instance);

}  // namespace medicat
