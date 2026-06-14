#pragma once

#include <string>
#include <vector>

namespace medicat {

std::wstring GetExeDirectory();
std::wstring JoinPath(const std::wstring& a, const std::wstring& b);
bool FileExists(const std::wstring& path);
std::wstring FormatBytes(uint64_t bytes);
std::wstring FormatPercent(int percent);
std::vector<std::wstring> SplitLines(const std::wstring& text);

}  // namespace medicat
