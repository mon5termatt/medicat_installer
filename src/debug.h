#pragma once

#include <functional>
#include <string>

namespace medicat {

constexpr wchar_t kDiagnosticSeparator[] = L"======================================";

struct DiagnosticContext {
    std::wstring outputDir;
    std::wstring operation;
    std::wstring selectedDrive;
    std::wstring sevenZaPath;
    std::wstring md5ManifestPath;
    std::wstring archivePath;
    bool formatChecked = false;
    bool runVentoyChecked = false;
    bool ventoySecureBoot = false;
    bool ventoyGpt = false;
    std::wstring pinnedVentoyVersion;
};

// Application + system + bundled tools (logged before session start line).
void LogSystemDiagnostics(const DiagnosticContext& context,
                          const std::function<void(const std::wstring&)>& logLine);

// Installer options for the selected drive (logged after the drive list is ready).
void LogInstallerDiagnostics(const DiagnosticContext& context,
                             const std::function<void(const std::wstring&)>& logLine);

struct SessionSystemSnapshot {
    int windowsBuild = 0;
    int windowsUbr = 0;
    std::string windowsMajorMinor;
    std::string editionId;
    std::string installationType;
    std::string processorArch;
    int logicalProcessors = 0;
    std::string ramGbBucket;
    std::string locale;
};

SessionSystemSnapshot CollectSessionSystemSnapshot();

}  // namespace medicat
