#pragma once

#include "gui.h"
#include "log.h"
#include "ventoy.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace medicat {

class App {
public:
    explicit App(HINSTANCE instance);
    int Run();

private:
    struct VerificationOutcome {
        bool success = false;
        std::wstring message;
        std::wstring title;
    };

    void OnInstall();
    void OnVerify();
    void PostProgress(int percent, bool clearLog = false);
    void PostExtractProgress(int percent, const std::wstring& file = L"", bool resetLog = false);
    void PostDone(bool success, const std::wstring& message, const std::wstring& title = L"");
    std::wstring WriteErrorDebugLog(const std::wstring& message, const std::wstring& title);
    VerificationOutcome VerifyDriveFiles(const std::wstring& drive, bool showFileProgress = true);
    void RunInstallThread(std::wstring drive, bool format, bool skipVentoy, std::wstring pinVersion,
                          VentoyInstallOptions ventoyInstall, HWND hwnd);
    void RunVerifyThread(std::wstring drive);

    HINSTANCE instance_;
    Gui gui_;
    std::unique_ptr<Logger> log_;
    std::wstring root_;
    std::wstring sevenZa_;
    std::wstring sevenZ_;
    std::wstring md5Manifest_;
    std::wstring currentOperation_;
    std::atomic<bool> installing_{false};
};

}  // namespace medicat
