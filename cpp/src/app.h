#pragma once

#include "gui.h"
#include "log.h"

#include <atomic>
#include <memory>
#include <thread>

namespace medicat {

class App {
public:
    explicit App(HINSTANCE instance);
    int Run();

private:
    void OnInstall();
    void PostProgress(int percent, const std::wstring& status);
    void PostDone(bool success, const std::wstring& message);
    void RunInstallThread(std::wstring drive, bool format, bool skipVentoy);

    HINSTANCE instance_;
    Gui gui_;
    std::unique_ptr<Logger> log_;
    std::wstring root_;
    std::wstring sevenZa_;
    std::wstring sevenZ_;
    std::atomic<bool> installing_{false};
};

}  // namespace medicat
