#pragma once

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

namespace medicat {

struct VentoyResult {
    enum class FailureKind { None, Download, Extract, Layout, Rename, Prepare };

    bool success = false;
    int exitCode = -1;
    FailureKind failureKind = FailureKind::None;
    std::wstring error;
    std::wstring cliLogExcerpt;  // last meaningful lines from Ventoy cli_log.txt
    std::wstring ventoyExe;
    std::wstring version;
};

struct VentoyEnsureOptions {
    std::wstring root;
    std::wstring sevenZipExe;
    std::wstring pinVersion;  // empty = always fetch/use latest
    std::wstring logPath;     // optional ventoy.log beside installer
    std::function<void(const std::wstring&)> onStatus;
    std::function<void(const std::wstring&)> onLog;
};

struct VentoyInstallOptions {
    bool useGpt = false;           // default MBR; append /GPT when true
    bool enableSecureBoot = true;  // Ventoy default; append /NOSB when false
    std::wstring logPath;          // optional ventoy.log beside installer (append)
};

// Fetch newest tag from GitHub (e.g. "1.0.99").
VentoyResult FetchLatestVentoyVersion(std::wstring& version);

// Fetch all release tags from GitHub (newest first).
VentoyResult FetchVentoyVersions(std::vector<std::wstring>& versions);

// Load compile-time embedded Ventoy version list.
bool LoadBundledVentoyVersionList(HINSTANCE instance, std::vector<std::wstring>& versions);

// Download/extract Ventoy if missing or version mismatch.
VentoyResult EnsureVentoyReady(const VentoyEnsureOptions& options);

struct VentoyDetectionResult {
    bool installed = false;
    std::vector<std::wstring> logLines;  // English diagnostics for medicat_installer.log
};

VentoyDetectionResult DetectVentoyOnDrive(const std::wstring& driveLetter,
                                          const std::wstring& logPath = L"");
bool TestVentoyInstalled(const std::wstring& driveLetter);

VentoyResult RunVentoyInstall(const std::wstring& ventoyExe, const std::wstring& driveLetter, bool upgrade,
                              const VentoyInstallOptions& options = {});

bool FormatDriveNtfs(const std::wstring& driveLetter, const std::wstring& label = L"Medicat");

}  // namespace medicat
