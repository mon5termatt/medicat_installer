#pragma once

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

namespace medicat {

struct VentoyResult {
    bool success = false;
    int exitCode = -1;
    std::wstring error;
    std::wstring ventoyExe;
    std::wstring version;
};

struct VentoyEnsureOptions {
    std::wstring root;
    std::wstring sevenZipExe;
    std::wstring pinVersion;  // empty = always fetch/use latest
    std::function<void(const std::wstring&)> onStatus;
    std::function<void(const std::wstring&)> onLog;
};

struct VentoyInstallOptions {
    bool useGpt = false;           // default MBR; append /GPT when true
    bool enableSecureBoot = true;  // Ventoy default; append /NOSB when false
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

VentoyDetectionResult DetectVentoyOnDrive(const std::wstring& driveLetter);
bool TestVentoyInstalled(const std::wstring& driveLetter);

VentoyResult RunVentoyInstall(const std::wstring& ventoyExe, const std::wstring& driveLetter, bool upgrade,
                              const VentoyInstallOptions& options = {});

bool FormatDriveNtfs(const std::wstring& driveLetter, const std::wstring& label = L"Medicat");

}  // namespace medicat
