#pragma once

#include "debug.h"
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
        size_t failedFiles = 0;
        std::vector<std::wstring> failures;
    };

    void OnInstall();
    void OnVerify();
    void PostProgress(int percent, bool clearLog = false);
    void PostExtractProgress(int percent, const std::wstring& file = L"", bool resetLog = false);
    void PostStatusBar(const std::wstring& text);  // Thread-safe; updates Gui::SetStatusBar on the UI thread.
    void PostDone(bool success, const std::wstring& message, const std::wstring& title = L"");
    void LogOperationFailure(const std::wstring& message, const std::wstring& title);
    DiagnosticContext BuildDiagnosticContext() const;
    VerificationOutcome VerifyDriveFiles(const std::wstring& drive, bool showFileProgress = true);
    bool TryReExtractFailedFiles(const std::wstring& drive, const std::wstring& archive,
                                 const std::vector<std::wstring>& failureDetails);
    bool PromptReExtract(const VerificationOutcome& outcome);
    void RunInstallThread(std::wstring drive, bool format, bool runVentoy, std::wstring pinVersion,
                          VentoyInstallOptions ventoyInstall, HWND hwnd);
    void RunVerifyThread(std::wstring drive);

    HINSTANCE instance_;
    Gui gui_;
    std::unique_ptr<Logger> log_;
    std::wstring root_;
    std::wstring sevenZa_;
    std::wstring md5Manifest_;
    std::wstring currentOperation_;
    std::atomic<bool> installing_{false};
};

}  // namespace medicat
