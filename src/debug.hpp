#pragma once

#include <string>

namespace medicat {

struct DebugReportContext {
    std::wstring outputDir;
    std::wstring errorMessage;
    std::wstring errorTitle;
    std::wstring operation;
    std::wstring selectedDrive;
    std::wstring installerLogPath;
    std::wstring sevenZaPath;
    std::wstring sevenZPath;
    std::wstring md5ManifestPath;
    std::wstring archivePath;
    bool formatChecked = false;
    bool runVentoyChecked = false;
    bool ventoySecureBoot = false;
    bool ventoyGpt = false;
    std::wstring pinnedVentoyVersion;
};

// Writes debug.log beside the installer. Returns the path written, or empty on failure.
std::wstring WriteDebugLog(const DebugReportContext& context);

}  // namespace medicat
