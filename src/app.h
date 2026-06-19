#pragma once

#include "cli.h"
#include "debug.h"
#include "gui.h"
#include "log.h"
#include "update.h"
#include "ventoy.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace medicat {

int RunApp(HINSTANCE instance);

class App {
public:
    explicit App(HINSTANCE instance, const std::wstring& logPath = L"");
    int RunParsed(const CliParseResult& parsed, int argc, wchar_t** argv);

private:
    struct VerificationOutcome {
        bool success = false;
        std::wstring message;
        std::wstring title;
        size_t failedFiles = 0;
        std::vector<std::wstring> failures;
    };

    struct HeadlessResult {
        bool completed = false;
        int exitCode = 1;
    };

    void OnInstall();
    void OnVerify();
    void PostProgress(int percent, bool clearLog = false);
    void PostExtractProgress(int percent, const std::wstring& file = L"", bool resetLog = false,
                             const wchar_t* cliTipKey = nullptr);
    void PostStatusBar(const std::wstring& text);  // Thread-safe; updates Gui::SetStatusBar on the UI thread.
    void PostDone(bool success, const std::wstring& message, const std::wstring& title = L"");
    void BeginAppSession();
    void MarkOperationStart();
    void SubmitLaunchSessionReport();
    void SubmitSessionReport(bool success, const std::wstring& message, const std::wstring& title, int exitCode);
    void QueueFailureLogUpload(const std::string& sessionId, const std::wstring& message, const std::wstring& title);
    void StartUpdateCheck();
    void LogOperationFailure(const std::wstring& message, const std::wstring& title);
    DiagnosticContext BuildDiagnosticContext() const;
    VerificationOutcome VerifyDriveFiles(const std::wstring& drive, bool showFileProgress = true);
    bool TryReExtractFailedFiles(const std::wstring& drive, const std::wstring& archive,
                                 const std::vector<std::wstring>& failureDetails,
                                 std::wstring* errorDetail = nullptr);
    bool PromptReExtract(const VerificationOutcome& outcome);
    void RunInstallThread(std::wstring drive, bool format, bool runVentoy, std::wstring pinVersion,
                          VentoyInstallOptions ventoyInstall, HWND hwnd);
    void RunVerifyThread(std::wstring drive);

    int RunGui();
    int RunHeadless(const CliOptions& cli);
    int RunHeadlessInstall(const CliOptions& cli);
    int RunHeadlessVerify(const CliOptions& cli);
    bool ValidateHeadlessDrive(const CliOptions& cli, std::wstring& errorMessage) const;
    bool ResolveHeadlessInstallOptions(const CliOptions& cli, bool& format, bool& runVentoy,
                                       VentoyInstallOptions& ventoyInstall, std::wstring& pinVersion,
                                       std::wstring& errorMessage) const;
    int MapHeadlessExitCode(bool success, const std::wstring& message, const std::wstring& title) const;
    void LogCommandLine(int argc, wchar_t** argv);
    std::wstring ResolveArchivePath(const std::wstring& overridePath) const;
    bool ShouldAutoConfirm() const;
    bool IsQuiet() const;
    bool WantsReExtract() const;

    HINSTANCE instance_;
    Gui gui_;
    std::unique_ptr<Logger> log_;
    std::wstring root_;
    std::wstring sevenZa_;
    std::wstring md5Manifest_;
    std::wstring currentOperation_;
    std::atomic<bool> installing_{false};
    bool headless_ = false;
    std::wstring cliProgressFile_;
    std::optional<CliOptions> cliOptions_;
    HeadlessResult headlessResult_;
    std::string sessionId_;
    std::chrono::steady_clock::time_point sessionStart_{};
    std::atomic<bool> updateCheckStarted_{false};
};

}  // namespace medicat
