#pragma once

#include "resource.h"

#include <windows.h>

#include <string>

namespace medicat {

struct BundledTools {
    std::wstring dir;
    std::wstring sevenZa;
    std::wstring sevenZ;
    bool ok = false;
    std::wstring error;
};

// Extract embedded 7za.exe and 7z.exe to %TEMP%\MedicatInstaller\{pid}\ (cached by size).
BundledTools EnsureBundledTools(HINSTANCE instance);

}  // namespace medicat
