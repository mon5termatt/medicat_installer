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

private:
    void Write(const std::wstring& level, const std::wstring& message);

    std::wstring path_;
    std::mutex mutex_;
};

}  // namespace medicat
