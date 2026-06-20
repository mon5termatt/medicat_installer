#pragma once

#include <string>
#include <vector>

namespace medicat {

std::wstring GetExeDirectory();
std::wstring GetMedicatTempRoot();
std::wstring GetMedicatTempDir();
void CleanupMedicatTempOnExit();
std::wstring JoinPath(const std::wstring& a, const std::wstring& b);
bool FileExists(const std::wstring& path);
uint64_t GetFileSizeBytes(const std::wstring& path);
std::wstring FormatBytes(uint64_t bytes);
std::wstring FormatProgressBytes(uint64_t bytes);
std::wstring FormatDownloadSpeed(uint64_t bytesPerSecond);
std::wstring FormatPercent(int percent);
std::wstring ShortDisplayPath(const std::wstring& path, size_t maxLen = 60);
std::vector<std::wstring> SplitLines(const std::wstring& text);

bool IsProcessElevated();

std::string WideToUtf8(const std::wstring& text);

std::wstring Utf8ToWide(const std::string& text);

extern const char kInstallerVersion[];
extern const int kInstallerBuildNumber;

std::wstring InstallerVersionWide();
std::wstring InstallerVersionLabel();

}  // namespace medicat
