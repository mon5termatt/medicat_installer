#include "util.hpp"

#include <shlwapi.h>
#include <windows.h>

#include <sstream>

namespace medicat {

std::wstring GetExeDirectory() {
    wchar_t buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return L".";
    }
    std::wstring path(buf, n);
    const auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, pos);
}

std::wstring GetMedicatTempDir() {
    wchar_t temp[MAX_PATH]{};
    const DWORD len = GetTempPathW(MAX_PATH, temp);
    if (len == 0 || len >= MAX_PATH) {
        return JoinPath(GetExeDirectory(), L"MedicatInstaller");
    }

    std::wstring root = JoinPath(std::wstring(temp, len), L"MedicatInstaller");
    CreateDirectoryW(root.c_str(), nullptr);

    std::wstring dir = JoinPath(root, std::to_wstring(GetCurrentProcessId()));
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::wstring JoinPath(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) {
        return b;
    }
    wchar_t out[MAX_PATH]{};
    if (PathCombineW(out, a.c_str(), b.c_str())) {
        return out;
    }
    if (a.back() == L'\\' || a.back() == L'/') {
        return a + b;
    }
    return a + L'\\' + b;
}

bool FileExists(const std::wstring& path) {
    const DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring FormatBytes(uint64_t bytes) {
    const double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gb >= 1.0) {
        std::wostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(0);
        ss << gb << L" GB";
        return ss.str();
    }
    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    std::wostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(1);
    ss << mb << L" MB";
    return ss.str();
}

std::wstring FormatPercent(int percent) {
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    return std::to_wstring(percent) + L"%";
}

std::wstring ShortDisplayPath(const std::wstring& path, const size_t maxLen) {
    if (path.size() <= maxLen) {
        return path;
    }
    return L"..." + path.substr(path.size() - (maxLen - 3));
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    std::wstring line;
    for (wchar_t ch : text) {
        if (ch == L'\n' || ch == L'\r') {
            if (!line.empty()) {
                lines.push_back(line);
                line.clear();
            }
        } else if (ch != L'\b') {
            line.push_back(ch);
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return lines;
}

}  // namespace medicat
