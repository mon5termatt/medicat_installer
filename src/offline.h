#pragma once

#include <string>
#include <vector>

namespace medicat {

// offline/ beside the installer exe — cached downloads for air-gapped use.
std::wstring GetOfflineDirectory();
std::wstring GetOfflineVentoyDirectory();
std::wstring GetOfflineVentoyZipPath(const std::wstring& version);

// ventoy-{version}-windows.zip in offline/ventoy/
bool OfflineVentoyZipExists(const std::wstring& version);

// Copy a downloaded zip into offline/ventoy/ (best-effort).
void CacheVentoyZip(const std::wstring& version, const std::wstring& zipPath);

// Load offline/ventoy_versions.txt (newest first).
bool LoadOfflineVentoyVersionList(std::vector<std::wstring>& versions);

// Newest version available from offline/ventoy/*.zip filenames.
bool FindNewestCachedVentoyVersion(std::wstring& version);

// Resolve target version when GitHub is unreachable (pin, cache, or version list).
bool ResolveOfflineVentoyVersion(const std::wstring& pinVersion, std::wstring& version);

// True when Ventoy can be prepared without network (local install dir or offline zip).
bool CanInstallVentoyOffline(const std::wstring& installerRoot, const std::wstring& pinVersion);

// MediCat.7z or MedicatFiles.md5 in offline/.
std::wstring ResolveOfflineArchivePath(const std::wstring& archiveName);
std::wstring ResolveOfflineMd5Manifest();

}  // namespace medicat
