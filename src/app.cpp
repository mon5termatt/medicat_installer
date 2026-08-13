#include <cwctype>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "app.h"

#include "archive.h"
#include "bundle.h"
#include "cancel.h"
#include "cli.h"
#include "debug.h"
#include "download.h"
#include "drives.h"
#include "extract.h"
#include "failure_tracker.h"
#include "i18n.h"
#include "offline.h"
#include "sim_fail.h"
#include "support.h"
#include "util.h"
#include "ventoy.h"
#include "verify.h"

namespace medicat {

namespace {

void PostToGui(HWND hwnd, UINT msg, LPARAM payload) {
    if (hwnd) {
        PostMessageW(hwnd, msg, 0, payload);
    } else if (payload) {
        if (msg == WM_MEDICAT_PROGRESS) {
            delete reinterpret_cast<ProgressPayload*>(payload);
        } else if (msg == WM_MEDICAT_DONE) {
            delete reinterpret_cast<DonePayload*>(payload);
        } else if (msg == WM_MEDICAT_REEXTRACT_PROMPT) {
            delete reinterpret_cast<ReExtractPromptPayload*>(payload);
        } else if (msg == WM_MEDICAT_UPDATE_RESULT) {
            delete reinterpret_cast<UpdateResultPayload*>(payload);
        } else if (msg == WM_MEDICAT_FAILURE_DIAG) {
            delete reinterpret_cast<FailureDiagPayload*>(payload);
        } else if (msg == WM_MEDICAT_CONFIRM_PROMPT) {
            delete reinterpret_cast<ConfirmPromptPayload*>(payload);
        }
    }
}

std::wstring BuildWipeDetails(const bool format, const bool runVentoy) {
    std::wstring details;
    if (runVentoy) {
        details += i18n::Tr(L"wipe_confirm.detail_ventoy");
        details += L"\n";
    }
    if (format) {
        details += i18n::Tr(L"wipe_confirm.detail_format");
        details += L"\n";
    }
    details += i18n::Tr(L"wipe_confirm.detail_extract");
    return details;
}

SessionReportLogger TelemetryFileLogger(Logger* log) {
    return [log](const std::wstring& line, const bool isError) {
        if (!log) {
            return;
        }
        const std::wstring prefixed = L"[Telemetry] " + line;
        if (isError) {
            log->Error(prefixed);
        } else {
            log->Info(prefixed);
        }
    };
}

}  // namespace

struct MedicatTempDirGuard {
    ~MedicatTempDirGuard() { CleanupMedicatTempOnExit(); }
};

App::App(HINSTANCE instance, const std::wstring& logPath) : instance_(instance) {
    i18n::Load();
    root_ = GetExeDirectory();
    const std::wstring resolvedLog = logPath.empty() ? JoinPath(root_, L"medicat_installer.log") : logPath;
    log_ = std::make_unique<Logger>(resolvedLog);

    const BundledTools tools = EnsureBundledTools(instance_);
    if (!tools.ok) {
        log_->Error(tools.error);
    } else {
        sevenZa_ = tools.sevenZa;
        md5Manifest_ = tools.md5Manifest;
        aria2c_ = tools.aria2c;
        SetAria2cPath(tools.aria2c);
        if (tools.aria2c.empty()) {
            log_->Info(L"aria2c not available; file downloads will use WinHTTP");
        }
    }
}

void App::LogCommandLine(int argc, wchar_t** argv) {
    if (argc <= 1) {
        return;
    }
    std::wstring joined;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            joined += L' ';
        }
        joined += argv[i];
    }
    log_->Info(L"Command line: " + joined);
}

std::wstring App::ResolveArchivePath(const std::wstring& overridePath) const {
    if (!overridePath.empty()) {
        return NormalizeMediCatArchivePath(overridePath);
    }
    if (!headless_) {
        const std::wstring browsed = gui_.SelectedArchivePath();
        if (!browsed.empty()) {
            return NormalizeMediCatArchivePath(browsed);
        }
    }
    return ResolveMediCatArchivePath(root_);
}

void App::LogMediCatArchiveDebug(const std::wstring& path) const {
    const std::wstring line = BuildMediCatArchiveSizeDebugLine(path);
    if (!line.empty()) {
        log_->Debug(line);
    }
}

bool App::ShouldAutoConfirm() const {
    return cliOptions_.has_value() && cliOptions_->yes;
}

bool App::IsQuiet() const {
    return cliOptions_.has_value() && cliOptions_->quiet;
}

bool App::WantsReExtract() const {
    if (!cliOptions_.has_value()) {
        return false;
    }
    switch (cliOptions_->reextract) {
        case CliReextractPolicy::Reextract:
        case CliReextractPolicy::ReextractOnly:
            return true;
        case CliReextractPolicy::NoReextract:
            return false;
        case CliReextractPolicy::Default:
        default:
            return cliOptions_->yes;
    }
}

int App::MapHeadlessExitCode(const bool success, const std::wstring& message, const std::wstring& title) const {
    if (IsCancelRequested()) {
        return 4;
    }
    if (success) {
        return 0;
    }
    if (message.empty()) {
        return 4;
    }
    if (title == i18n::Tr(L"titles.verification_failed")) {
        return 5;
    }
    if (title == i18n::Tr(L"titles.medicat_not_on_drive") ||
        title == i18n::Tr(L"titles.verification_all_failed_wrong_drive")) {
        return 7;
    }
    if (title == i18n::Tr(L"titles.verify_still_failed_after_reextract")) {
        return 6;
    }
    return 1;
}

bool App::ValidateHeadlessDrive(const CliOptions& cli, std::wstring& errorMessage) const {
    if (cli.drive.empty()) {
        errorMessage = L"Drive letter is required";
        return false;
    }

    if (cli.drive.size() >= 1 && towupper(cli.drive[0]) == L'C') {
        errorMessage = L"The system drive C: cannot be used";
        return false;
    }

    if (!MeetsMinimumDriveCapacity(cli.drive)) {
        errorMessage = i18n::Tr(L"messages.drive_under_minimum", cli.drive);
        return false;
    }

    const std::vector<DriveInfo> drives = ListTargetDrives(cli.allowFixed);
    for (const DriveInfo& info : drives) {
        if (_wcsicmp(info.letter.c_str(), cli.drive.c_str()) == 0) {
            return true;
        }
    }

    errorMessage = L"Drive not eligible: " + cli.drive;
    return false;
}

bool App::ResolveHeadlessInstallOptions(const CliOptions& cli, bool& format, bool& runVentoy,
                                        VentoyInstallOptions& ventoyInstall, std::wstring& pinVersion,
                                        std::wstring& errorMessage) const {
    const bool ventoyOnDrive = TestVentoyInstalled(cli.drive);
    const bool forced = !ventoyOnDrive;

    ventoyInstall.useGpt = cli.gpt.value_or(false);
    ventoyInstall.enableSecureBoot = cli.secureBoot.value_or(true);
    pinVersion = cli.ventoyVersion;

    if (forced) {
        if (cli.format.has_value() && !*cli.format) {
            errorMessage = L"/noformat cannot be used when Ventoy is not installed on the drive";
            return false;
        }
        if (cli.runVentoy.has_value() && !*cli.runVentoy) {
            errorMessage = L"/noventoy cannot be used when Ventoy is not installed on the drive";
            return false;
        }
        format = true;
        runVentoy = true;
        return true;
    }

    format = cli.format.value_or(false);
    runVentoy = cli.runVentoy.value_or(false);
    return true;
}

int App::RunParsed(const CliParseResult& parsed, int argc, wchar_t** argv) {
    MedicatTempDirGuard tempCleanup;

    const bool cliMode =
        parsed.options.action == CliAction::Install || parsed.options.action == CliAction::Verify;
    if (cliMode) {
        EnableConsoleCancelHandling();
        log_->SetConsoleMirror(true, parsed.options.quiet);
    }

    LogCommandLine(argc, argv);
    LogSystemDiagnostics(BuildDiagnosticContext(),
                         [this](const std::wstring& line) { log_->Info(line); },
                         [this](const std::wstring& line) { log_->Debug(line); });
    log_->Info(L"MediCat Installer (C++) started");

    if (parsed.options.action == CliAction::Help) {
        PrintCliHelp();
        return 0;
    }
    if (parsed.options.action == CliAction::Version) {
        PrintCliVersion();
        return 0;
    }
    if (parsed.options.action == CliAction::ListDrives) {
        PrintCliDrives(parsed.options.allowFixed);
        return 0;
    }
    if (parsed.options.action == CliAction::DumpConfig) {
        PrintCliConfig(root_, sevenZa_, aria2c_, md5Manifest_, ResolveArchivePath(parsed.options.archivePath));
        return 0;
    }

    if (parsed.options.action == CliAction::Install || parsed.options.action == CliAction::Verify) {
        headless_ = true;
    }

    BeginAppSession();
    SubmitLaunchSessionReport();

    if (sevenZa_.empty()) {
        currentOperation_ = L"startup";
        LogOperationFailure(i18n::Tr(L"messages.7zip_not_found"), i18n::Tr(L"titles.7zip_not_found"));
        if (parsed.options.action == CliAction::Install || parsed.options.action == CliAction::Verify) {
            return 1;
        }
        MessageBoxW(nullptr, i18n::Tr(L"messages.7zip_not_found").c_str(),
                    i18n::Tr(L"titles.7zip_not_found").c_str(), MB_ICONERROR);
        return 1;
    }

    if (parsed.options.action == CliAction::Install || parsed.options.action == CliAction::Verify) {
        cliOptions_ = parsed.options;
        return RunHeadless(parsed.options);
    }

    if (!parsed.options.language.empty()) {
        cliOptions_ = parsed.options;
    }
    return RunGui();
}

int App::RunGui() {
    gui_.SetLogHandler([this](const std::wstring& msg) { log_->Info(msg); });

    if (!gui_.Create(instance_)) {
        return 1;
    }

    if (cliOptions_.has_value() && !cliOptions_->language.empty()) {
        gui_.SetInitialLanguage(cliOptions_->language);
    }

    gui_.SetInstallHandler([this] { OnInstall(); });
    gui_.SetVerifyHandler([this] { OnVerify(); });
    gui_.SetUpdateCheckHandler([this] { StartUpdateCheck(); });
    gui_.SetApplyInstallerUpdateHandler([this](const InstallerUpdateInfo& info) { ApplyInstallerUpdate(info); });
    gui_.ScheduleUpdateCheck();
    LogInstallerDiagnostics(BuildDiagnosticContext(),
                            [this](const std::wstring& line) { log_->Info(line); });
    return gui_.Run();
}

int App::RunHeadless(const CliOptions& cli) {
    headless_ = true;
    ResetCancelState();

    std::wstring errorMessage;
    if (!ValidateHeadlessDrive(cli, errorMessage)) {
        log_->Error(errorMessage);
        return 2;
    }

    if (cli.action == CliAction::Verify) {
        return RunHeadlessVerify(cli);
    }
    return RunHeadlessInstall(cli);
}

int App::RunHeadlessVerify(const CliOptions& cli) {
    installing_ = true;
    currentOperation_ = L"verify";
    MarkOperationStart();
    log_->Info(L"Headless verify started on " + cli.drive);
    RunVerifyThread(cli.drive);
    return headlessResult_.completed ? headlessResult_.exitCode : 1;
}

int App::RunHeadlessInstall(const CliOptions& cli) {
    if (!IsProcessElevated()) {
        const std::wstring msg = i18n::Tr(L"errors.elevation_required");
        log_->Error(msg);
        return 3;
    }

    bool format = false;
    bool runVentoy = false;
    VentoyInstallOptions ventoyInstall;
    std::wstring pinVersion;
    std::wstring errorMessage;
    if (!ResolveHeadlessInstallOptions(cli, format, runVentoy, ventoyInstall, pinVersion, errorMessage)) {
        log_->Error(errorMessage);
        return 2;
    }

    const std::wstring archive = ResolveArchivePath(cli.archivePath);
    LogMediCatArchiveDebug(archive);

    if (cli.offlineOnly && runVentoy && !CanInstallVentoyOffline(root_, pinVersion)) {
        const std::wstring msg = L"Offline Ventoy cache not available for the requested install";
        log_->Error(msg);
        return 1;
    }

    installing_ = true;
    currentOperation_ = L"install";
    log_->Info(L"Headless install started on " + cli.drive);
    RunPreInstallThread(cli.drive, format, runVentoy, std::move(pinVersion), ventoyInstall, std::move(archive));
    return headlessResult_.completed ? headlessResult_.exitCode : 1;
}

DiagnosticContext App::BuildDiagnosticContext() const {
    DiagnosticContext context;
    context.outputDir = root_;
    context.sevenZaPath = sevenZa_;
    context.aria2cPath = aria2c_;
    context.md5ManifestPath = md5Manifest_;
    context.archivePath = ResolveArchivePath(cliOptions_.has_value() ? cliOptions_->archivePath : L"");
    context.operation = currentOperation_;
    if (cliOptions_.has_value() && headless_) {
        context.selectedDrive = cliOptions_->drive;
        bool format = false;
        bool runVentoy = false;
        VentoyInstallOptions ventoyInstall;
        std::wstring pinVersion;
        std::wstring ignoredError;
        if (ResolveHeadlessInstallOptions(*cliOptions_, format, runVentoy, ventoyInstall, pinVersion, ignoredError)) {
            context.formatChecked = format;
            context.runVentoyChecked = runVentoy;
            context.ventoySecureBoot = ventoyInstall.enableSecureBoot;
            context.ventoyGpt = ventoyInstall.useGpt;
            context.pinnedVentoyVersion = pinVersion;
        }
    } else if (gui_.Hwnd()) {
        context.selectedDrive = gui_.SelectedDrive();
        context.formatChecked = gui_.FormatChecked();
        context.runVentoyChecked = gui_.RunVentoyChecked();
        context.ventoySecureBoot = gui_.VentoySecureBootChecked();
        context.ventoyGpt = gui_.VentoyGptChecked();
        context.pinnedVentoyVersion = gui_.PinnedVentoyVersion();
    }
    return context;
}

void App::LogOperationFailure(const std::wstring& message, const std::wstring& title) {
    log_->Info(kDiagnosticSeparator);
    log_->Info(L"[Failure]");
    if (!title.empty()) {
        log_->Info(L"Error title: " + title);
    }
    if (!message.empty()) {
        log_->Error(message);
    }
    LogInstallerDiagnostics(BuildDiagnosticContext(),
                            [this](const std::wstring& line) { log_->Info(line); });
}

void App::PostProgress(const int percent, const bool clearLog) {
    auto* payload = new ProgressPayload{};
    payload->percent = percent;
    payload->clearLog = clearLog;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostExtractProgress(const int percent, const std::wstring& file, const bool resetLog,
                              const wchar_t* cliTipKey) {
    if (headless_) {
        if (resetLog) {
            cliProgressFile_.clear();
            if (cliTipKey) {
                WriteCliTip(i18n::Tr(cliTipKey));
            }
            return;
        }
        if (!file.empty()) {
            cliProgressFile_ = file;
        }
        WriteCliProgress(FormatCliFileProgress(percent, cliProgressFile_));
        return;
    }

    auto* payload = new ProgressPayload{};
    payload->percent = percent;
    payload->resetLog = resetLog;
    payload->extractUpdate = true;
    payload->file = file;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostStatusBar(const std::wstring& text) {
    auto* payload = new ProgressPayload{};
    payload->statusOnly = true;
    payload->statusText = text;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostArchiveHashProgress(const uint64_t bytesRead, const uint64_t totalBytes) {
    static thread_local uint64_t lastUiTick = 0;
    const uint64_t now = GetTickCount64();
    if (lastUiTick != 0 && now - lastUiTick < 200) {
        return;
    }
    lastUiTick = now;

    int percent = 0;
    if (totalBytes > 0) {
        percent = static_cast<int>((bytesRead * 100) / totalBytes);
    }

    if (headless_) {
        std::wstring line = FormatProgressBytes(bytesRead);
        if (totalBytes > 0) {
            line += L" / " + FormatProgressBytes(totalBytes);
        }
        WriteCliProgress(line);
        return;
    }

    std::wstring sizeText = FormatProgressBytes(bytesRead);
    if (totalBytes > 0) {
        sizeText += L" / " + FormatProgressBytes(totalBytes);
    }

    auto* payload = new ProgressPayload{};
    payload->downloadUpdate = true;
    payload->percent = percent;
    payload->statusText = sizeText;
    payload->file = i18n::Tr(L"status.verifying_archive");
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostArchiveCheckFailed(const std::wstring& message, const std::wstring& title) {
    installing_ = false;
    if (headless_) {
        headlessResult_.completed = true;
        headlessResult_.exitCode = 1;
        log_->Error(message);
        return;
    }

    auto* payload = new DonePayload{};
    payload->success = false;
    payload->message = message;
    payload->title = title;
    payload->refreshArchivePanel = true;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_DONE, reinterpret_cast<LPARAM>(payload));
}

void App::PostSetBusyMode(const BusyProgressMode mode) {
    if (headless_) {
        return;
    }
    auto* payload = new ProgressPayload{};
    payload->setBusyMode = true;
    payload->busyProgressMode = mode;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostOpenFileLog() {
    if (headless_) {
        return;
    }
    auto* payload = new ProgressPayload{};
    payload->openFileLog = true;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostFailureDiagCode(const bool uploadSucceeded, const std::wstring& keyword) {
    if (uploadSucceeded && !keyword.empty()) {
        log_->Info(L"[Telemetry] Support diag code: " + keyword);
    }
    if (headless_) {
        if (uploadSucceeded && !keyword.empty()) {
            std::wcerr << L"Diag code: " << keyword << L"\n";
        }
        return;
    }
    auto* payload = new FailureDiagPayload{};
    payload->uploadSucceeded = uploadSucceeded;
    payload->keyword = keyword;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_FAILURE_DIAG, reinterpret_cast<LPARAM>(payload));
}

void App::BeginAppSession() {
    if (sessionId_.empty()) {
        sessionId_ = GenerateSessionId();
        sessionStart_ = std::chrono::steady_clock::now();
    }
}

void App::MarkOperationStart() {
    BeginAppSession();
    sessionStart_ = std::chrono::steady_clock::now();
}

void App::SubmitLaunchSessionReport() {
    if (sessionId_.empty()) {
        return;
    }

    SessionReportRequest request;
    request.sessionId = sessionId_;
    request.operation = "launch";
    request.outcome = "opened";
    request.exitCode = 0;
    request.durationMs = 0;
    request.headless = headless_;
    request.diagnostic = BuildDiagnosticContext();

    SendSessionReport(request, false, TelemetryFileLogger(log_.get()));
}

void App::SubmitSessionReport(const bool success, const std::wstring& message, const std::wstring& title,
                              const int exitCode) {
    if (sessionId_.empty()) {
        return;
    }

    int64_t durationMs = 0;
    if (sessionStart_.time_since_epoch().count() != 0) {
        durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                           sessionStart_)
                         .count();
    }

    SessionReportRequest request;
    request.sessionId = sessionId_;
    request.operation = WideToUtf8(currentOperation_);
    request.outcome = DeriveSessionOutcome(success, message, title, currentOperation_);
    request.exitCode = exitCode;
    request.durationMs = durationMs;
    request.headless = headless_;
    request.diagnostic = BuildDiagnosticContext();

    if (!success && !message.empty()) {
        std::wstring errorTitle = title;
        if (errorTitle.empty()) {
            errorTitle = currentOperation_ == L"verify" ? i18n::Tr(L"titles.verification_failed")
                                                        : i18n::Tr(L"titles.installation_error");
        }
        request.errorTitle = SanitizeTelemetryTextEnglish(errorTitle, 128);
        request.errorDetail = SanitizeTelemetryTextEnglish(message, 900);
    }

    SendSessionReport(request, headless_, TelemetryFileLogger(log_.get()));
    sessionId_.clear();
    sessionStart_ = {};
}

void App::QueueFailureLogUpload(const std::string& sessionId, const std::wstring& message,
                                const std::wstring& title) {
    if (sessionId.empty() || sevenZa_.empty()) {
        return;
    }

    FailureLogUploadRequest request;
    request.sessionId = sessionId;
    request.operation = WideToUtf8(currentOperation_);
    request.installerRoot = root_;
    request.sevenZa = sevenZa_;
    request.diagnostic = BuildDiagnosticContext();
    if (!message.empty() || !title.empty()) {
        std::wstring errorTitle = title;
        if (errorTitle.empty()) {
            errorTitle = currentOperation_ == L"verify" ? i18n::Tr(L"titles.verification_failed")
                                                        : i18n::Tr(L"titles.installation_error");
        }
        request.errorTitle = SanitizeTelemetryTextEnglish(errorTitle, 128);
        request.errorDetail = SanitizeTelemetryTextEnglish(message, 900);
    }

    SendFailureLogUpload(
        request, TelemetryFileLogger(log_.get()),
        [this](const bool success, const std::wstring& keyword) { PostFailureDiagCode(success, keyword); });
}

void App::StartUpdateCheck() {
    if (installing_.load()) {
        return;
    }

    bool expected = false;
    if (!updateCheckInProgress_.compare_exchange_strong(expected, true)) {
        return;
    }

    HWND hwnd = gui_.Hwnd();
    std::thread([this, hwnd]() {
        const auto finish = [this] { updateCheckInProgress_ = false; };
        if (installing_.load()) {
            finish();
            return;
        }

        std::wstring connectionError;
        if (!TestInternetConnection(connectionError)) {
            log_->Debug(L"[Update] Skipped — offline");
            finish();
            return;
        }

        log_->Info(i18n::Tr(L"update.checking"));
        const UpdateCheckResult result = CheckForInstallerUpdate();
        if (!result.success) {
            log_->Debug(L"[Update] Check failed — " + result.error);
            finish();
            return;
        }
        if (!result.info.updateAvailable) {
            log_->Debug(L"[Update] Installer is up to date (local " + InstallerVersionWide() + L", remote " +
                        result.info.releaseTag + L", remoteBuild " + std::to_wstring(result.info.remoteBuild) + L")");
            finish();
            return;
        }

        log_->Info(L"[Update] Newer installer available — v" + result.info.version + L" (" + result.info.releaseTag +
                   L", remoteBuild " + std::to_wstring(result.info.remoteBuild) + L", localBuild " +
                   std::to_wstring(kInstallerBuildNumber) + L")");
        auto* payload = new UpdateResultPayload{result.info};
        PostToGui(hwnd, WM_MEDICAT_UPDATE_RESULT, reinterpret_cast<LPARAM>(payload));
        finish();
    }).detach();
}

void App::ApplyInstallerUpdate(const InstallerUpdateInfo& info) {
    if (installing_.load()) {
        return;
    }
    installing_ = true;
    gui_.SetBusy(true, BusyProgressMode::Download);
    gui_.SetDownloadProgress(0, FormatProgressBytes(0), i18n::Tr(L"update.downloading"));

    HWND hwnd = gui_.Hwnd();
    std::thread([this, hwnd, info]() {
        uint64_t lastUiTick = 0;
        std::wstring error;
        const bool ok = DownloadAndRelaunchInstallerUpdate(
            info,
            [&](const uint64_t downloaded, const uint64_t total) {
                const uint64_t now = GetTickCount64();
                if (lastUiTick != 0 && now - lastUiTick < 200) {
                    return;
                }
                lastUiTick = now;

                int percent = 0;
                if (total > 0) {
                    percent = static_cast<int>((downloaded * 100) / total);
                }
                std::wstring sizeText = FormatProgressBytes(downloaded);
                if (total > 0) {
                    sizeText += L" / " + FormatProgressBytes(total);
                }

                auto* progress = new ProgressPayload{};
                progress->downloadUpdate = true;
                progress->percent = percent;
                progress->statusText = sizeText;
                progress->file = i18n::Tr(L"update.downloading");
                PostToGui(hwnd, WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(progress));
            },
            [this](const std::wstring& line) { log_->Info(line); }, error);

        if (!ok) {
            installing_ = false;
            auto* payload = new DonePayload{};
            payload->success = false;
            payload->message = i18n::Tr(L"update.download_failed", error);
            payload->title = i18n::Tr(L"update.download_failed_title");
            PostToGui(hwnd, WM_MEDICAT_DONE, reinterpret_cast<LPARAM>(payload));
            return;
        }

        log_->Info(i18n::Tr(L"update.restarting"));
        ExitProcess(0);
    }).detach();
}

void App::PostDone(const bool success, const std::wstring& message, const std::wstring& title) {
    installing_ = false;
    if (!success && !message.empty()) {
        RecordOperationFailure();
        LogOperationFailure(message, title);
    } else if (success) {
        RecordOperationSuccess();
    }
    const int exitCode = headless_ ? MapHeadlessExitCode(success, message, title) : (success ? 0 : 1);
    if (headless_) {
        headlessResult_.completed = true;
        headlessResult_.exitCode = exitCode;
    }
    const std::string failureSessionId = sessionId_;
    if (!success && !message.empty()) {
        QueueFailureLogUpload(failureSessionId, message, title);
    }
    SubmitSessionReport(success, message, title, exitCode);
    if (headless_) {
        return;
    }

    if (!success && !message.empty()) {
        gui_.ResetFailureDiagCode();
    }

    std::wstring userMessage = message;
    if (!success && !message.empty()) {
        userMessage += L"\n\n" + i18n::Tr(L"messages.beta_failure_logs_notice");
    }
    auto* payload = new DonePayload{};
    payload->success = success;
    payload->message = userMessage;
    payload->title = title;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_DONE, reinterpret_cast<LPARAM>(payload));
}

App::VerificationOutcome App::VerifyDriveFiles(const std::wstring& drive, const bool showFileProgress) {
    VerificationOutcome outcome;

    std::wstring dest = drive;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }

    log_->Info(i18n::Tr(L"log.file_check_started", drive));
    if (!md5Manifest_.empty()) {
        log_->Debug(L"Using MD5 manifest: " + md5Manifest_);
    }

    PostStatusBar(i18n::Tr(L"status.checking_medicat_presence"));
    log_->Info(i18n::Tr(L"log.medicat_presence_check", drive));
    const MedicatPresenceResult presence = CheckMedicatPresenceOnDrive(dest);
    log_->Info(i18n::Tr(L"log.medicat_presence_score", std::to_wstring(presence.scorePercent),
                        std::to_wstring(presence.markersFound), std::to_wstring(presence.markersTotal)));
    for (const std::wstring& marker : presence.foundMarkers) {
        log_->Debug(i18n::Tr(L"log.medicat_presence_marker_found", marker));
    }
    for (const std::wstring& marker : presence.missingMarkers) {
        log_->Debug(i18n::Tr(L"log.medicat_presence_marker_missing", marker));
    }
    if (!presence.likelyInstalled) {
        log_->Error(i18n::Tr(L"log.medicat_presence_too_low", std::to_wstring(presence.scorePercent),
                             std::to_wstring(kMedicatPresenceProceedThresholdPercent)));
        outcome.skipReExtract = true;
        outcome.message =
            i18n::Tr(L"messages.medicat_not_on_drive", drive, std::to_wstring(presence.scorePercent),
                     std::to_wstring(presence.markersFound), std::to_wstring(presence.markersTotal));
        outcome.title = i18n::Tr(L"titles.medicat_not_on_drive");
        return outcome;
    }
    log_->Info(i18n::Tr(L"log.medicat_presence_ok", std::to_wstring(presence.scorePercent)));
    if (showFileProgress) {
        PostOpenFileLog();
    }

    VerifyOptions verifyOptions;
    verifyOptions.driveRoot = dest;
    verifyOptions.installerRoot = root_;
    verifyOptions.tempDir = GetMedicatTempDir();
    verifyOptions.manifestPath = md5Manifest_;
    verifyOptions.failedListPath = JoinPath(root_, L"failed_files.txt");
    verifyOptions.checkLogPath = JoinPath(root_, L"check.log");
    log_->Info(L"Writing per-file verify log to " + verifyOptions.checkLogPath);

    if (headless_) {
        WriteCliTip(i18n::Tr(L"status.verifying_files"));
        verifyOptions.onProgress = [this](const VerifyProgress& progress) {
            if (progress.total == 0) {
                return;
            }
            const int percent = static_cast<int>((progress.current * 100) / progress.total);
            WriteCliProgress(FormatCliFileProgress(percent, progress.file));
        };
    } else if (showFileProgress) {
        verifyOptions.onFileLog = [this](const VerifyFileLogEntry& entry) {
            if (entry.total == 0) {
                return;
            }
            const int percent = static_cast<int>((entry.current * 100) / entry.total);
            PostExtractProgress(percent, entry.line, false);
        };
    }
    verifyOptions.onLog = [this](const std::wstring& msg) { log_->Info(msg); };

    const VerifyResult verify = VerifyMedicatFiles(verifyOptions);
    if (headless_) {
        WriteCliProgressFinish();
    }
    if (IsCancelRequested()) {
        outcome.message.clear();
        outcome.title.clear();
        return outcome;
    }
    log_->Debug(L"VerifyMedicatFiles returned, total=" + std::to_wstring(verify.totalFiles) + L" verified=" +
                std::to_wstring(verify.verifiedFiles) + L" failed=" + std::to_wstring(verify.failedFiles));
    if (!verify.error.empty() && verify.totalFiles == 0) {
        const bool manifestMissing =
            verify.error.find(L"no valid manifest") != std::wstring::npos ||
            verify.error.find(L"no file entries") != std::wstring::npos ||
            verify.error.find(L"HTTP request returned status") != std::wstring::npos;
        outcome.message = manifestMissing ? i18n::Tr(L"messages.md5_manifest_missing")
                                          : i18n::Tr(L"messages.verification_error", verify.error);
        outcome.title = i18n::Tr(L"titles.verification_error");
        return outcome;
    }

    if (!verify.error.empty()) {
        log_->Error(verify.error);
        outcome.message = i18n::Tr(L"messages.verification_error", verify.error);
        outcome.title = i18n::Tr(L"titles.verification_error");
        return outcome;
    }

    log_->Info(i18n::Tr(L"log.verification_complete", std::to_wstring(verify.verifiedFiles),
                        std::to_wstring(verify.totalFiles)));

    if (verify.failedFiles > 0) {
        log_->Error(i18n::Tr(L"log.files_failed", std::to_wstring(verify.failedFiles)));
        for (size_t i = 0; i < verify.failures.size() && i < 25; ++i) {
            log_->Error(L"  - " + verify.failures[i]);
        }
        if (!verify.failedListPath.empty()) {
            log_->Info(L"Failed files list written to: " + verify.failedListPath);
        }
        if (!verify.checkLogPath.empty()) {
            log_->Info(L"Per-file verify log: " + verify.checkLogPath);
        }
        outcome.failedFiles = verify.failedFiles;
        outcome.failures = verify.failures;

        const bool allFailed =
            verify.verifiedFiles == 0 && verify.failedFiles > 0 && verify.failedFiles >= verify.totalFiles;
        if (allFailed) {
            outcome.skipReExtract = true;
            outcome.message = i18n::Tr(L"messages.verification_all_failed_wrong_drive", drive,
                                        std::to_wstring(verify.failedFiles));
            outcome.title = i18n::Tr(L"titles.verification_all_failed_wrong_drive");
            log_->Error(outcome.message);
            return outcome;
        }

        outcome.message = i18n::Tr(L"messages.verification_failed", std::to_wstring(verify.failedFiles));
        outcome.title = i18n::Tr(L"titles.verification_failed");
        return outcome;
    }

    outcome.success = true;
    outcome.message = i18n::Tr(L"messages.verification_complete", std::to_wstring(verify.verifiedFiles),
                               std::to_wstring(verify.totalFiles));
    if (!verify.checkLogPath.empty()) {
        log_->Info(L"Per-file verify log: " + verify.checkLogPath);
    }
    if (verify.skippedFiles > 0) {
        outcome.message += L"\n\n" + i18n::Tr(L"messages.verify_skipped_files", std::to_wstring(verify.skippedFiles));
    }
    outcome.title = i18n::Tr(L"titles.verification_complete");
    return outcome;
}

namespace {

std::wstring RelativePathFromFailureDetail(const std::wstring& detail) {
    const size_t pos = detail.find(L" (");
    if (pos == std::wstring::npos) {
        return detail;
    }
    return detail.substr(0, pos);
}

std::wstring ExtractDestinationFailureMessage(const ExtractResult& extract, const std::wstring& drive) {
    if (extract.driveRemoved) {
        return i18n::Tr(L"messages.drive_removed_during_operation", drive);
    }
    if (extract.ioError) {
        std::wstring msg = i18n::Tr(L"messages.drive_io_error_during_operation", drive);
        if (!extract.detail.empty()) {
            msg += L"\n\n" + extract.detail;
        }
        return msg;
    }
    return {};
}

std::wstring FormatVentoyReadyFailure(const VentoyResult& ready) {
    switch (ready.failureKind) {
        case VentoyResult::FailureKind::Download:
            return FormatDetailedError(i18n::Tr(L"errors.ventoy_download_failed"), ready.error);
        case VentoyResult::FailureKind::Extract: {
            const std::wstring summary =
                i18n::Tr(L"errors.ventoy_extract_failed", std::to_wstring(ready.exitCode >= 0 ? ready.exitCode : 0));
            return FormatDetailedError(summary, ready.error);
        }
        case VentoyResult::FailureKind::Layout:
            return FormatDetailedError(i18n::Tr(L"errors.ventoy_layout_failed"), ready.error);
        case VentoyResult::FailureKind::Rename:
            return FormatDetailedError(i18n::Tr(L"errors.ventoy_rename_failed"), ready.error);
        case VentoyResult::FailureKind::Prepare:
            return FormatDetailedError(i18n::Tr(L"errors.ventoy_not_found"), ready.error);
        default:
            return ready.error.empty() ? i18n::Tr(L"errors.ventoy_not_found") : ready.error;
    }
}

std::wstring VentoyReadyFailureTitle(const VentoyResult& ready) {
    if (ready.failureKind == VentoyResult::FailureKind::Extract) {
        return i18n::Tr(L"titles.extraction_failed");
    }
    if (ready.failureKind == VentoyResult::FailureKind::Rename ||
        ready.failureKind == VentoyResult::FailureKind::Layout) {
        return i18n::Tr(L"titles.ventoy_prepare_failed");
    }
    return i18n::Tr(L"titles.download_failed");
}

bool TriggerSimulatedInstallFailure(const SimulatedFailure kind, Logger* log,
                                    const std::function<void(const std::wstring&, const std::wstring&)>& fail) {
    if (!ConsumeSimulatedFailure(kind)) {
        return false;
    }
    const std::optional<SimulatedInstallFailure> simulated = MakeSimulatedInstallFailure(kind);
    if (!simulated) {
        return false;
    }
    log->Info(std::wstring(L"[Debug] Simulating: ") + SimulatedFailureLabel(kind));
    fail(simulated->message, simulated->title);
    return true;
}

}  // namespace

bool App::TryReExtractFailedFiles(const std::wstring& drive, const std::wstring& archive,
                                  const std::vector<std::wstring>& failureDetails,
                                  std::wstring* errorDetail) {
    if (drive.empty() || archive.empty() || failureDetails.empty()) {
        return false;
    }

    std::wstring dest = drive;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }

    std::vector<std::wstring> relPaths;
    relPaths.reserve(failureDetails.size());
    for (const auto& detail : failureDetails) {
        const std::wstring rel = RelativePathFromFailureDetail(detail);
        if (!rel.empty()) {
            relPaths.push_back(rel);
        }
    }
    if (relPaths.empty()) {
        return false;
    }

    LogMediCatArchiveDebug(archive);
    std::wstring archiveMessage;
    std::wstring archiveTitle;
    if (!IsMediCatArchiveReadyForInstall(
            archive, archiveMessage, archiveTitle,
            [this](const std::wstring& status) { PostStatusBar(status); },
            [this](const std::wstring& msg) { log_->Info(msg); })) {
        log_->Error(archiveMessage);
        if (errorDetail) {
            *errorDetail = archiveMessage;
        }
        return false;
    }

    log_->Info(i18n::Tr(L"log.re_extraction_started", std::to_wstring(relPaths.size())));
    PostStatusBar(i18n::Tr(L"status.re_extracting"));
    PostExtractProgress(0, L"", true, L"status.re_extracting");

    const std::wstring extractLogPath = JoinPath(root_, L"reextract.log");
    log_->Info(L"Writing selective 7za output to " + extractLogPath);

    const ExtractResult extract = Extract7zArchiveSelective(
        sevenZa_, archive, dest, relPaths,
        [this](const ExtractProgress& p) { PostExtractProgress(p.percent, p.file, false); }, extractLogPath);
    if (headless_) {
        WriteCliProgressFinish();
    }

    if (!extract.success) {
        if (const std::wstring destinationFailure = ExtractDestinationFailureMessage(extract, drive);
            !destinationFailure.empty()) {
            log_->Error(destinationFailure);
            if (errorDetail) {
                *errorDetail = destinationFailure;
            }
            return false;
        }
        if (extract.cancelled || IsCancelRequested()) {
            log_->Info(i18n::Tr(L"log.user_cancelled"));
            return false;
        }
        const std::wstring failureMessage = FormatExtractFailureMessage(extract);
        log_->Error(L"Selective re-extract failed: " + extract.error);
        if (errorDetail) {
            *errorDetail = failureMessage;
        }
        return false;
    }

    return true;
}

bool App::PromptReExtract(const VerificationOutcome& outcome) {
    if (headless_) {
        (void)outcome;
        return WantsReExtract();
    }

    auto state = std::make_shared<ReExtractPromptState>();
    state->doneEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!state->doneEvent) {
        return false;
    }

    auto* payload = new ReExtractPromptPayload{};
    payload->message = outcome.message;
    payload->title = outcome.title;
    payload->failedFiles = outcome.failedFiles;
    payload->failures = outcome.failures;
    payload->state = state;

    if (!PostMessageW(gui_.Hwnd(), WM_MEDICAT_REEXTRACT_PROMPT, 0, reinterpret_cast<LPARAM>(payload))) {
        delete payload;
        CloseHandle(state->doneEvent);
        return false;
    }

    WaitForSingleObject(state->doneEvent, INFINITE);
    const bool wantReExtract = state->wantReExtract.load();
    CloseHandle(state->doneEvent);
    return wantReExtract;
}

bool App::PromptConfirm(const std::wstring& message, const std::wstring& title, const MessageDialogKind kind) {
    if (headless_) {
        return false;
    }

    auto state = std::make_shared<ConfirmPromptState>();
    state->doneEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!state->doneEvent) {
        return false;
    }

    auto* payload = new ConfirmPromptPayload{};
    payload->message = message;
    payload->title = title;
    payload->kind = kind;
    payload->state = state;

    if (!PostMessageW(gui_.Hwnd(), WM_MEDICAT_CONFIRM_PROMPT, 0, reinterpret_cast<LPARAM>(payload))) {
        delete payload;
        CloseHandle(state->doneEvent);
        return false;
    }

    WaitForSingleObject(state->doneEvent, INFINITE);
    const bool confirmed = state->result.load();
    CloseHandle(state->doneEvent);
    return confirmed;
}

bool App::PromptWipeConfirm(const std::wstring& drive, const bool format, const bool runVentoy) {
    if (ShouldAutoConfirm()) {
        return true;
    }
    if (IsQuiet()) {
        return false;
    }

    const std::wstring details = BuildWipeDetails(format, runVentoy);
    const std::wstring message = i18n::Tr(L"wipe_confirm.message", drive, details);
    const std::wstring title = i18n::Tr(L"wipe_confirm.title");

    if (headless_) {
        return MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) ==
               IDYES;
    }
    return PromptConfirm(message, title, MessageDialogKind::Warning);
}

bool App::EnsureHelpGateAcknowledged() {
    if (!NeedsHelpGate()) {
        return true;
    }

    const int failures = OperationFailureCount();
    log_->Info(L"Help gate shown after " + std::to_wstring(failures) + L" operation failure(s)");
    return gui_.ShowHelpGateDialog(failures);
}

void App::OnVerify() {
    if (installing_.exchange(true)) {
        return;
    }

    if (!EnsureHelpGateAcknowledged()) {
        installing_ = false;
        return;
    }

    const std::wstring drive = gui_.SelectedDrive();
    if (drive.empty()) {
        gui_.ShowMessageDialog(i18n::Tr(L"messages.no_drive_selected"), i18n::Tr(L"titles.no_drive_selected"),
                               MessageDialogKind::Warning);
        installing_ = false;
        return;
    }

    if (!gui_.MedicatOnSelectedDrive()) {
        // Verify is greyed when presence fails; ignore stray clicks during/after detection.
        installing_ = false;
        return;
    }

    if (!MeetsMinimumDriveCapacity(drive)) {
        gui_.ShowMessageDialog(i18n::Tr(L"messages.drive_under_minimum", drive),
                               i18n::Tr(L"titles.drive_under_minimum"), MessageDialogKind::Warning);
        installing_ = false;
        return;
    }

    gui_.SetBusy(true, BusyProgressMode::Verify);
    gui_.ClearFileLog();
    gui_.SetProgress(0);
    gui_.SetStatusBar(i18n::Tr(L"status.verifying_files"));
    currentOperation_ = L"verify";
    MarkOperationStart();
    log_->Info(L"File verification started on " + drive);

    std::thread worker(&App::RunVerifyThread, this, drive);
    worker.detach();
}

void App::RunVerifyThread(std::wstring drive) {
    PostExtractProgress(0, L"", true);

    try {
        if (TriggerSimulatedInstallFailure(SimulatedFailure::VerificationFailed, log_.get(),
                                           [&](const std::wstring& msg, const std::wstring& title) {
                                               PostDone(false, msg, title);
                                           })) {
            return;
        }

        const VerificationOutcome outcome = VerifyDriveFiles(drive, true);
        if (IsCancelRequested() || (!outcome.success && outcome.message.empty() && outcome.title.empty())) {
            log_->Info(i18n::Tr(L"log.user_cancelled"));
            PostDone(false, L"");
            return;
        }
        if (!outcome.success) {
            log_->Error(outcome.message);

            // Offer selective re-extract when we have failures and the source archive is available.
            if (outcome.failedFiles > 0 && !outcome.failures.empty() && !outcome.skipReExtract) {
                const std::wstring archive = ResolveArchivePath(cliOptions_.has_value() ? cliOptions_->archivePath : L"");
                if (FileExists(archive) && PromptReExtract(outcome)) {
                    std::wstring reextractError;
                    const bool reextractOk =
                        TryReExtractFailedFiles(drive, archive, outcome.failures, &reextractError);
                    if (!reextractOk) {
                        if (IsCancelRequested()) {
                            log_->Info(i18n::Tr(L"log.user_cancelled"));
                            PostDone(false, L"");
                            return;
                        }
                        const std::wstring detail =
                            reextractError.empty() ? L"selective extract failed" : reextractError;
                        PostDone(false, i18n::Tr(L"messages.re_extraction_error", detail),
                                 i18n::Tr(L"titles.re_extraction_error"));
                        return;
                    }

                    const VerificationOutcome after = VerifyDriveFiles(drive, true);
                    if (IsCancelRequested() || (!after.success && after.message.empty() && after.title.empty())) {
                        log_->Info(i18n::Tr(L"log.user_cancelled"));
                        PostDone(false, L"");
                        return;
                    }
                    if (!after.success) {
                        const size_t stillFailed =
                            after.failedFiles > 0 ? after.failedFiles : after.failures.size();
                        PostDone(false,
                                 i18n::Tr(L"messages.verify_still_failed_after_reextract",
                                          std::to_wstring(stillFailed)),
                                 i18n::Tr(L"titles.verify_still_failed_after_reextract"));
                        return;
                    }

                    // Verification already succeeded after re-extract; show the normal success summary.
                    PostDone(true, after.message, after.title);
                    return;
                }
            }

            PostDone(false, outcome.message, outcome.title);
            return;
        }

        log_->Info(i18n::Tr(L"log.all_files_ok"));
        PostDone(true, outcome.message, outcome.title);
    } catch (...) {
        log_->Error(L"Verify thread crashed with an unexpected exception");
        PostDone(false, i18n::Tr(L"messages.verification_error", L"unexpected error"),
                 i18n::Tr(L"titles.verification_error"));
    }
}

void App::OnInstall() {
    if (installing_.exchange(true)) {
        return;
    }

    if (!EnsureHelpGateAcknowledged()) {
        installing_ = false;
        return;
    }

    const std::wstring drive = gui_.SelectedDrive();
    if (drive.empty()) {
        gui_.ShowMessageDialog(i18n::Tr(L"messages.no_drive_selected"), i18n::Tr(L"titles.no_drive_selected"),
                               MessageDialogKind::Warning);
        installing_ = false;
        return;
    }

    if (!MeetsMinimumDriveCapacity(drive)) {
        gui_.ShowMessageDialog(i18n::Tr(L"messages.drive_under_minimum", drive),
                               i18n::Tr(L"titles.drive_under_minimum"), MessageDialogKind::Warning);
        installing_ = false;
        return;
    }

    const std::wstring archive = ResolveArchivePath(L"");
    LogMediCatArchiveDebug(archive);

    gui_.SetBusy(true, BusyProgressMode::Download);
    gui_.ClearFileLog();
    gui_.SetProgress(0);
    gui_.SetStatusBar(i18n::Tr(L"status.verifying_archive"));
    currentOperation_ = L"install";
    log_->Info(L"Install started on " + drive);

    const bool format = gui_.FormatChecked();
    const bool runVentoy = gui_.RunVentoyChecked();
    const std::wstring pinVersion = gui_.PinnedVentoyVersion();
    VentoyInstallOptions ventoyInstall;
    ventoyInstall.enableSecureBoot = gui_.VentoySecureBootChecked();
    ventoyInstall.useGpt = gui_.VentoyGptChecked();

    std::thread worker(&App::RunPreInstallThread, this, drive, format, runVentoy, pinVersion, ventoyInstall,
                       archive);
    worker.detach();
}

namespace {

void PostVentoyStatus(const std::function<void(int, bool)>& postProgress, const std::wstring& key) {
    (void)key;
    postProgress(0, false);
}

bool SameDriveLetter(const std::wstring& a, const std::wstring& b) {
    if (a.empty() || b.empty()) {
        return false;
    }
    const wchar_t letterA = towupper(a[0]);
    const wchar_t letterB = towupper(b[0]);
    return letterA == letterB;
}

bool ReconcileDriveLetter(
    std::wstring& drive, const DriveIdentity& identity, const std::function<void(const std::wstring&)>& log,
    const std::function<void()>& cancel, const std::function<void(const std::wstring&)>& fail,
    const wchar_t* contextLabel,
    const std::function<bool(const std::wstring& originalDrive, const std::wstring& newDrive)>& confirmLetterChange) {
    const std::wstring resolved = ResolveDriveLetterAfterVentoy(drive, identity);
    if (resolved.empty()) {
        fail(i18n::Tr(L"messages.drive_lost_after_ventoy"));
        return false;
    }

    if (!SameDriveLetter(resolved, drive)) {
        log(std::wstring(contextLabel) + L": drive letter changed from " + drive + L" to " + resolved);
        if (!confirmLetterChange(drive, resolved)) {
            cancel();
            return false;
        }
        drive = resolved;
    } else {
        log(std::wstring(contextLabel) + L": drive letter still " + drive);
    }

    return true;
}

}  // namespace

void App::RunPreInstallThread(std::wstring drive, const bool format, const bool runVentoy, std::wstring pinVersion,
                               VentoyInstallOptions ventoyInstall, std::wstring archive) {
    try {
        if (headless_) {
            WriteCliTip(i18n::Tr(L"status.verifying_archive"));
        }

        std::wstring archiveMessage;
        std::wstring archiveTitle;
        if (!IsMediCatArchiveReadyForInstall(
                archive, archiveMessage, archiveTitle,
                [this](const std::wstring& status) { PostStatusBar(status); },
                [this](const std::wstring& msg) { log_->Info(msg); },
                [this](const uint64_t bytesRead, const uint64_t totalBytes) {
                    PostArchiveHashProgress(bytesRead, totalBytes);
                })) {
            PostArchiveCheckFailed(archiveMessage, archiveTitle);
            return;
        }

        if (headless_) {
            WriteCliProgressFinish();
        }

        if (!PromptWipeConfirm(drive, format, runVentoy)) {
            log_->Info(L"User cancelled at wipe confirmation");
            installing_ = false;
            if (headless_) {
                headlessResult_.completed = true;
                headlessResult_.exitCode = 4;
                return;
            }
            auto* payload = new DonePayload{};
            payload->success = false;
            PostToGui(gui_.Hwnd(), WM_MEDICAT_DONE, reinterpret_cast<LPARAM>(payload));
            return;
        }

        PostSetBusyMode(BusyProgressMode::FileLog);
        MarkOperationStart();
        RunInstallThread(std::move(drive), format, runVentoy, std::move(pinVersion), ventoyInstall);
    } catch (...) {
        log_->Error(L"Pre-install thread crashed with an unexpected exception");
        PostDone(false, i18n::Tr(L"messages.installation_error", L"unexpected error"),
                 i18n::Tr(L"titles.installation_error"));
    }
}

void App::RunInstallThread(std::wstring drive, bool format, bool runVentoy, std::wstring pinVersion,
                            VentoyInstallOptions ventoyInstall) {
    const std::wstring root = root_;
    const std::wstring archive = ResolveArchivePath(cliOptions_.has_value() ? cliOptions_->archivePath : L"");
    const std::wstring sevenZip = sevenZa_;
    const bool autoYes = ShouldAutoConfirm();
    const bool quiet = IsQuiet();

    auto fail = [&](const std::wstring& msg, const std::wstring& title = L"") {
        log_->Error(msg);
        PostDone(false, msg, title);
    };

    LogMediCatArchiveDebug(archive);
    // std::wstring archiveMessage;
    // std::wstring archiveTitle;
    // if (!IsMediCatArchiveReadyForInstall(
    //         archive, archiveMessage, archiveTitle,
    //         [this](const std::wstring& status) { PostStatusBar(status); },
    //         [this](const std::wstring& msg) { log_->Info(msg); })) {
    //     fail(archiveMessage, archiveTitle);
    //     return;
    // }

    auto cancel = [&] {
        log_->Info(i18n::Tr(L"log.user_cancelled"));
        PostDone(false, L"");
    };

    const auto logInfo = [&](const std::wstring& msg) { log_->Info(msg); };

    const auto postProgress = [&](const int percent, const bool clearLog) {
        PostProgress(percent, clearLog);
    };

    const auto confirmVentoy = [&]() -> bool {
        if (autoYes) {
            return true;
        }
        if (quiet) {
            return false;
        }
        if (headless_) {
            const std::wstring message = i18n::Tr(L"ventoy_warning.message", drive);
            const std::wstring title = i18n::Tr(L"ventoy_warning.title");
            return MessageBoxW(nullptr, message.c_str(), title.c_str(),
                               MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
        }
        return PromptConfirm(i18n::Tr(L"ventoy_warning.message", drive), i18n::Tr(L"ventoy_warning.title"),
                             MessageDialogKind::Warning);
    };

    const auto confirmDriveLetterChange = [&](const std::wstring& originalDrive,
                                              const std::wstring& newDrive) -> bool {
        if (autoYes) {
            return true;
        }
        if (quiet) {
            return false;
        }
        if (headless_) {
            const std::wstring message = i18n::Tr(L"drive_letter_changed.message", originalDrive, newDrive);
            const std::wstring title = i18n::Tr(L"drive_letter_changed.title");
            return MessageBoxW(nullptr, message.c_str(), title.c_str(),
                               MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
        }
        return PromptConfirm(i18n::Tr(L"drive_letter_changed.message", originalDrive, newDrive),
                             i18n::Tr(L"drive_letter_changed.title"), MessageDialogKind::Warning);
    };

    std::wstring ventoyExe;
    DriveIdentity driveIdentity;
    if (IsCancelRequested()) {
        cancel();
        return;
    }
    if (runVentoy) {
        driveIdentity = GetDriveIdentity(drive);
        if (!driveIdentity.valid) {
            log_->Info(L"Could not read drive identity before Ventoy; letter remapping may be limited");
        }

        std::wstring netError;
        if (TriggerSimulatedInstallFailure(SimulatedFailure::NoInternet, log_.get(), fail)) {
            return;
        }
        if (!CanInstallVentoyOffline(root, pinVersion) && !TestInternetConnection(netError)) {
            log_->Error(L"Internet check failed: " + netError);
            fail(FormatDetailedError(i18n::Tr(L"messages.no_internet"), netError), i18n::Tr(L"titles.download_failed"));
            return;
        }

        const std::wstring ventoyLogPath = JoinPath(root, L"ventoy.log");
        log_->Info(L"Writing Ventoy diagnostics to " + ventoyLogPath);

        VentoyEnsureOptions ensureOptions;
        ensureOptions.root = root;
        ensureOptions.sevenZipExe = sevenZip;
        ensureOptions.pinVersion = std::move(pinVersion);
        ensureOptions.logPath = ventoyLogPath;
        ensureOptions.onStatus = [&](const std::wstring& key) { PostVentoyStatus(postProgress, key); };
        ensureOptions.onLog = [&](const std::wstring& msg) { log_->Info(msg); };

        const VentoyResult ready = EnsureVentoyReady(ensureOptions);
        if (!ready.success) {
            if (IsCancelRequested()) {
                cancel();
                return;
            }
            fail(FormatVentoyReadyFailure(ready), VentoyReadyFailureTitle(ready));
            return;
        }
        ventoyExe = ready.ventoyExe;
        log_->Info(L"Ventoy v" + ready.version + L" ready");

        PostProgress(0);
        const VentoyDetectionResult ventoyDetection = DetectVentoyOnDrive(drive, ventoyLogPath);
        for (const std::wstring& line : ventoyDetection.logLines) {
            log_->Info(line);
        }
        const bool ventoyInstalled = ventoyDetection.installed;
        if (ventoyInstalled) {
            log_->Info(i18n::Tr(L"log.ventoy_detected"));
        } else {
            log_->Info(i18n::Tr(L"log.ventoy_not_found"));
        }

        const bool destructiveVentoyInstall = format || !ventoyInstalled;
        const bool upgrade = !destructiveVentoyInstall;
        if (upgrade) {
            log_->Info(L"Ventoy detected - using in-place upgrade");
        } else if (!ventoyInstalled) {
            log_->Info(L"Ventoy not detected - destructive Ventoy install required");
        } else {
            log_->Info(i18n::Tr(L"log.format_enabled"));
        }

        if (!confirmVentoy()) {
            cancel();
            return;
        }

        PostProgress(0);
        log_->Info(upgrade ? L"Running Ventoy upgrade" : L"Running Ventoy fresh install");
        log_->Info(L"Ventoy options: " + std::wstring(ventoyInstall.useGpt ? L"GPT" : L"MBR") + L", Secure Boot " +
                   (ventoyInstall.enableSecureBoot ? L"enabled" : L"disabled"));
        if (TriggerSimulatedInstallFailure(upgrade ? SimulatedFailure::VentoyUpgrade : SimulatedFailure::VentoyInstall,
                                           log_.get(), fail)) {
            return;
        }
        ventoyInstall.logPath = ventoyLogPath;
        const VentoyResult ventoy = RunVentoyInstall(ventoyExe, drive, upgrade, ventoyInstall);
        if (!ventoy.success) {
            if (!ventoy.cliLogExcerpt.empty()) {
                log_->Error(L"Ventoy cli_log.txt excerpt:\n" + ventoy.cliLogExcerpt);
            } else if (!ventoy.error.empty()) {
                log_->Error(ventoy.error);
            }
            const std::wstring exitLine =
                i18n::Tr(L"messages.process_exit_code", L"Ventoy2Disk", std::to_wstring(ventoy.exitCode));
            std::wstring detail = exitLine;
            if (!ventoy.cliLogExcerpt.empty()) {
                detail += L"\n\n" + ventoy.cliLogExcerpt;
            }
            const std::wstring summary = upgrade ? i18n::Tr(L"messages.ventoy_upgrade_failed", detail)
                                                 : i18n::Tr(L"messages.ventoy_install_failed", detail);
            const std::wstring title =
                upgrade ? i18n::Tr(L"titles.ventoy_upgrade_failed") : i18n::Tr(L"titles.ventoy_install_failed");
            fail(summary, title);
            return;
        }

        if (!ReconcileDriveLetter(drive, driveIdentity, logInfo, cancel, fail, L"After Ventoy install",
                                  confirmDriveLetterChange)) {
            return;
        }

        log_->Info(L"Using drive " + drive + L" for format/extract");
    } else {
        log_->Info(L"Skipping Ventoy");
    }

    if (!MeetsMinimumDriveCapacity(drive)) {
        fail(i18n::Tr(L"messages.drive_under_minimum", drive));
        return;
    }

    if (format) {
        PostProgress(0);
        log_->Info(L"Formatting " + drive);
        if (TriggerSimulatedInstallFailure(SimulatedFailure::FormatFailed, log_.get(), fail)) {
            return;
        }
        if (!FormatDriveNtfs(drive)) {
            fail(FormatDetailedError(i18n::Tr(L"errors.format_failed", drive), L"NTFS format command failed."),
                 i18n::Tr(L"titles.installation_error"));
            return;
        }
    }

    if (runVentoy) {
        if (!ReconcileDriveLetter(drive, driveIdentity, logInfo, cancel, fail, L"Final check before extract",
                                  confirmDriveLetterChange)) {
            return;
        }
    }

    std::wstring dest = drive;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }

    PostProgress(0);
    const uint64_t totalBytes = GetArchiveUncompressedSize(sevenZip, archive);
    const uint64_t initialFree = GetDriveFreeBytes(dest);
    log_->Debug(L"Uncompressed size: " + FormatBytes(totalBytes));
    log_->Debug(L"Initial free space: " + FormatBytes(initialFree));

    if (totalBytes > 0 && initialFree > 0 && totalBytes > initialFree) {
        fail(i18n::Tr(L"messages.usb_too_small"));
        return;
    }

    PostExtractProgress(0, L"", true, L"status.extracting_archive");

    const std::wstring extractLogPath = JoinPath(root_, L"extract.log");
    log_->Info(L"Writing raw 7za output to " + extractLogPath);

    if (TriggerSimulatedInstallFailure(SimulatedFailure::MediCatExtract, log_.get(), fail)) {
        return;
    }

    const ExtractResult extract = Extract7zArchive(
        sevenZip, archive, dest, totalBytes, initialFree,
        [this](const ExtractProgress& p) { PostExtractProgress(p.percent, p.file, false); }, extractLogPath);
    if (headless_) {
        WriteCliProgressFinish();
    }

    if (!extract.success) {
        if (const std::wstring destinationFailure = ExtractDestinationFailureMessage(extract, drive);
            !destinationFailure.empty()) {
            fail(destinationFailure, i18n::Tr(L"titles.extraction_failed"));
            return;
        }
        if (extract.cancelled || IsCancelRequested()) {
            cancel();
            return;
        }
        fail(i18n::Tr(L"messages.extraction_failed", FormatExtractFailureMessage(extract)),
             i18n::Tr(L"titles.extraction_failed"));
        return;
    }

    PostProgress(0);
    const VerificationOutcome outcome = VerifyDriveFiles(drive);
    if (IsCancelRequested() || (!outcome.success && outcome.message.empty() && outcome.title.empty())) {
        cancel();
        return;
    }
    if (!outcome.success) {
        if (outcome.failedFiles > 0 && !outcome.failures.empty() && !outcome.skipReExtract) {
            if (PromptReExtract(outcome)) {
                std::wstring reextractError;
                const bool reextractOk =
                    TryReExtractFailedFiles(drive, archive, outcome.failures, &reextractError);
                if (!reextractOk) {
                    if (IsCancelRequested()) {
                        cancel();
                        return;
                    }
                    const std::wstring detail =
                        reextractError.empty() ? L"selective extract failed" : reextractError;
                    fail(i18n::Tr(L"messages.re_extraction_error", detail), i18n::Tr(L"titles.re_extraction_error"));
                    return;
                }

                PostProgress(0);
                const VerificationOutcome after = VerifyDriveFiles(drive, false);
                if (IsCancelRequested() || (!after.success && after.message.empty() && after.title.empty())) {
                    cancel();
                    return;
                }
                if (!after.success) {
                    const size_t stillFailed = after.failedFiles > 0 ? after.failedFiles : after.failures.size();
                    const std::wstring msg =
                        i18n::Tr(L"messages.verify_still_failed_after_reextract", std::to_wstring(stillFailed));
                    log_->Error(msg);
                    PostDone(false, msg, i18n::Tr(L"titles.verify_still_failed_after_reextract"));
                    return;
                }

                // Verification already succeeded after re-extract; show the normal success summary.
                PostDone(true, after.message, after.title);
                return;
            }
        }
        fail(outcome.message);
        return;
    }

    log_->Info(i18n::Tr(L"log.all_files_ok"));
    log_->Info(L"Install completed successfully");
    PostDone(true, i18n::Tr(L"messages.installation_complete", drive));
}

}  // namespace medicat
