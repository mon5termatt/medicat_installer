#include "log.h"

#include "cli.h"

#include <windows.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace medicat {

namespace {

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

std::wstring NowStamp() {
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::wostringstream ss;
    ss << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S");
    return ss.str();
}

}  // namespace

Logger::Logger(std::wstring path) : path_(std::move(path)) {
    std::ofstream out(WideToUtf8(path_), std::ios::app);
    out << "========================================\n";
    out << "MediCat Installer (C++) - " << WideToUtf8(NowStamp()) << "\n";
    out << "========================================\n";
}

void Logger::SetConsoleMirror(const bool enabled, const bool errorsOnly) {
    std::lock_guard lock(mutex_);
    consoleMirror_ = enabled;
    consoleErrorsOnly_ = errorsOnly;
    if (enabled) {
        AttachCliConsole();
    }
}

void Logger::Write(const std::wstring& level, const std::wstring& message) {
    std::lock_guard lock(mutex_);
    const std::wstring stamp = NowStamp();
    const std::wstring line = L"[" + stamp + L"] " + level + L": " + message;

    std::ofstream out(WideToUtf8(path_), std::ios::app);
    out << WideToUtf8(line) << "\n";
    out.flush();

    if (consoleMirror_ && (!consoleErrorsOnly_ || level == L"ERROR")) {
        WriteCliLine(line);
    }
}

void Logger::Info(const std::wstring& message) { Write(L"INFO", message); }
void Logger::Debug(const std::wstring& message) { Write(L"DEBUG", message); }
void Logger::Error(const std::wstring& message) { Write(L"ERROR", message); }

}  // namespace medicat
