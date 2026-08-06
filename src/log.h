#pragma once

#include <mutex>
#include <string>

namespace medicat {

class Logger {
public:
    explicit Logger(std::wstring path);
    void Info(const std::wstring& message);
    void Debug(const std::wstring& message);
    void Error(const std::wstring& message);
    // Mirror log lines to the attached console (CLI / headless mode).
    void SetConsoleMirror(bool enabled, bool errorsOnly = false);

private:
    void Write(const std::wstring& level, const std::wstring& message);

    std::wstring path_;
    std::mutex mutex_;
    bool consoleMirror_ = false;
    bool consoleErrorsOnly_ = false;
};

}  // namespace medicat
