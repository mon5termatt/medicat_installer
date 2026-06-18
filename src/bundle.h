#pragma once

#include "resource.h"

#include <windows.h>

#include <string>

namespace medicat {

struct BundledTools {
    std::wstring dir;
    std::wstring sevenZa;
    std::wstring md5Manifest;
    bool ok = false;
    std::wstring error;
};

// Extract embedded 7za.exe (arch-selected) and MedicatFiles.md5 to %TEMP%\MedicatInstaller\{pid}\.
BundledTools EnsureBundledTools(HINSTANCE instance);

}  // namespace medicat
