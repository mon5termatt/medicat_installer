#include "app.h"

#include "bundle.h"
#include "debug.h"
#include "download.h"
#include "drives.h"
#include "extract.h"
#include "i18n.h"
#include "offline.h"
#include "util.h"
#include "ventoy.h"
#include "verify.h"

#include <cwctype>
#include <functional>
#include <memory>
#include <sstream>

namespace medicat {

constexpr wchar_t kMediCatArchiveName[] = L"MediCat.USB.v21.12.7z";

std::wstring ResolveMediCatArchivePath(const std::wstring& root) {
    const std::wstring besideExe = JoinPath(root, kMediCatArchiveName);
    if (FileExists(besideExe)) {
        return besideExe;
    }

    const std::wstring offline = ResolveOfflineArchivePath(kMediCatArchiveName);
    if (!offline.empty()) {
        return offline;
    }

    return besideExe;
}

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

bool ConfirmWipeDrive(HWND hwnd, const std::wstring& drive, const bool format, const bool runVentoy) {
    const std::wstring details = BuildWipeDetails(format, runVentoy);
    const int result = MessageBoxW(
        hwnd, i18n::Tr(L"wipe_confirm.message", drive, details).c_str(),
        i18n::Tr(L"wipe_confirm.title").c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    return result == IDYES;
}

}  // namespace

struct MedicatTempDirGuard {
    ~MedicatTempDirGuard() { CleanupMedicatTempOnExit(); }
};

App::App(HINSTANCE instance) : instance_(instance) {
    i18n::Load();
    root_ = GetExeDirectory();
    log_ = std::make_unique<Logger>(JoinPath(root_, L"medicat_installer.log"));

    const BundledTools tools = EnsureBundledTools(instance_);
    if (!tools.ok) {
        log_->Error(tools.error);
    } else {
        sevenZa_ = tools.sevenZa;
        md5Manifest_ = tools.md5Manifest;
    }
}

int App::Run() {
    MedicatTempDirGuard tempCleanup;
    LogSystemDiagnostics(BuildDiagnosticContext(),
                         [this](const std::wstring& line) { log_->Info(line); });
    log_->Info(L"MediCat Installer (C++) started");
    gui_.SetLogHandler([this](const std::wstring& msg) { log_->Info(msg); });

    if (!gui_.Create(instance_)) {
        return 1;
    }

    if (sevenZa_.empty()) {
        currentOperation_ = L"startup";
        LogOperationFailure(i18n::Tr(L"messages.7zip_not_found"), i18n::Tr(L"titles.7zip_not_found"));
        MessageBoxW(gui_.Hwnd(), i18n::Tr(L"messages.7zip_not_found").c_str(),
                    i18n::Tr(L"titles.7zip_not_found").c_str(), MB_ICONERROR);
        return 1;
    }

    gui_.SetInstallHandler([this] { OnInstall(); });
    gui_.SetVerifyHandler([this] { OnVerify(); });
    LogInstallerDiagnostics(BuildDiagnosticContext(),
                            [this](const std::wstring& line) { log_->Info(line); });
    return gui_.Run();
}

DiagnosticContext App::BuildDiagnosticContext() const {
    DiagnosticContext context;
    context.outputDir = root_;
    context.sevenZaPath = sevenZa_;
    context.md5ManifestPath = md5Manifest_;
    context.archivePath = ResolveMediCatArchivePath(root_);
    context.operation = currentOperation_;
    if (gui_.Hwnd()) {
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
    log_->Info(L"Error message: " + message);
    LogInstallerDiagnostics(BuildDiagnosticContext(),
                            [this](const std::wstring& line) { log_->Info(line); });
}

void App::PostProgress(const int percent, const bool clearLog) {
    auto* payload = new ProgressPayload{};
    payload->percent = percent;
    payload->clearLog = clearLog;
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostExtractProgress(const int percent, const std::wstring& file, const bool resetLog) {
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

void App::PostDone(const bool success, const std::wstring& message, const std::wstring& title) {
    installing_ = false;
    if (!success && !message.empty()) {
        LogOperationFailure(message, title);
    }
    auto* payload = new DonePayload{success, message, title};
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

    VerifyOptions verifyOptions;
    verifyOptions.driveRoot = dest;
    verifyOptions.installerRoot = root_;
    verifyOptions.tempDir = GetMedicatTempDir();
    verifyOptions.manifestPath = md5Manifest_;
    verifyOptions.failedListPath = JoinPath(root_, L"failed_files.txt");
    verifyOptions.checkLogPath = JoinPath(root_, L"check.log");
    log_->Info(L"Writing per-file verify log to " + verifyOptions.checkLogPath);
    if (showFileProgress) {
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

}  // namespace

bool App::TryReExtractFailedFiles(const std::wstring& drive, const std::wstring& archive,
                                  const std::vector<std::wstring>& failureDetails) {
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

    log_->Info(i18n::Tr(L"log.re_extraction_started", std::to_wstring(relPaths.size())));
    PostStatusBar(i18n::Tr(L"status.re_extracting"));
    PostExtractProgress(0, L"", true);

    const std::wstring extractLogPath = JoinPath(root_, L"reextract.log");
    log_->Info(L"Writing selective 7za output to " + extractLogPath);

    const ExtractResult extract = Extract7zArchiveSelective(
        sevenZa_, archive, dest, relPaths,
        [this](const ExtractProgress& p) { PostExtractProgress(p.percent, p.file, false); }, extractLogPath);

    if (!extract.success) {
        log_->Error(L"Selective re-extract failed: " + (extract.error.empty() ? L"unknown error" : extract.error));
        return false;
    }

    return true;
}

bool App::PromptReExtract(const VerificationOutcome& outcome) {
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

void App::OnVerify() {
    if (installing_.exchange(true)) {
        return;
    }

    const std::wstring drive = gui_.SelectedDrive();
    if (drive.empty()) {
        MessageBoxW(gui_.Hwnd(), i18n::Tr(L"messages.no_drive_selected").c_str(),
                    i18n::Tr(L"titles.no_drive_selected").c_str(), MB_ICONWARNING);
        installing_ = false;
        return;
    }

    if (!MeetsMinimumDriveCapacity(drive)) {
        MessageBoxW(gui_.Hwnd(), i18n::Tr(L"messages.drive_under_minimum", drive).c_str(),
                    i18n::Tr(L"titles.drive_under_minimum").c_str(), MB_ICONWARNING);
        installing_ = false;
        return;
    }

    gui_.SetBusy(true, BusyProgressMode::Verify);
    gui_.ClearFileLog();
    gui_.OpenFileLogWindow();
    gui_.SetProgress(0);
    gui_.SetStatusBar(i18n::Tr(L"status.verifying_files"));
    currentOperation_ = L"verify";
    log_->Info(L"File verification started on " + drive);

    std::thread worker(&App::RunVerifyThread, this, drive);
    worker.detach();
}

void App::RunVerifyThread(std::wstring drive) {
    PostExtractProgress(0, L"", true);

    try {
        const VerificationOutcome outcome = VerifyDriveFiles(drive, true);
        if (!outcome.success) {
            log_->Error(outcome.message);

            // Offer selective re-extract when we have failures and the source archive is available.
            if (outcome.failedFiles > 0 && !outcome.failures.empty()) {
                const std::wstring archive = ResolveMediCatArchivePath(root_);
                if (FileExists(archive) && PromptReExtract(outcome)) {
                    const bool reextractOk = TryReExtractFailedFiles(drive, archive, outcome.failures);
                    if (!reextractOk) {
                        PostDone(false, i18n::Tr(L"messages.re_extraction_error", L"selective extract failed"),
                                 i18n::Tr(L"titles.re_extraction_error"));
                        return;
                    }

                    const VerificationOutcome after = VerifyDriveFiles(drive, true);
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

    const std::wstring drive = gui_.SelectedDrive();
    if (drive.empty()) {
        MessageBoxW(gui_.Hwnd(), i18n::Tr(L"messages.no_drive_selected").c_str(),
                    i18n::Tr(L"titles.no_drive_selected").c_str(), MB_ICONWARNING);
        installing_ = false;
        return;
    }

    if (!MeetsMinimumDriveCapacity(drive)) {
        MessageBoxW(gui_.Hwnd(), i18n::Tr(L"messages.drive_under_minimum", drive).c_str(),
                    i18n::Tr(L"titles.drive_under_minimum").c_str(), MB_ICONWARNING);
        installing_ = false;
        return;
    }

    const std::wstring archive = ResolveMediCatArchivePath(root_);
    if (!FileExists(archive)) {
        installing_ = false;
        return;
    }

    gui_.SetBusy(true);
    gui_.ClearFileLog();
    gui_.SetProgress(0);
    currentOperation_ = L"install";
    log_->Info(L"Install started on " + drive);

    const bool format = gui_.FormatChecked();
    const bool runVentoy = gui_.RunVentoyChecked();

    if (!ConfirmWipeDrive(gui_.Hwnd(), drive, format, runVentoy)) {
        log_->Info(L"User cancelled at wipe confirmation");
        installing_ = false;
        gui_.SetBusy(false);
        return;
    }

    const std::wstring pinVersion = gui_.PinnedVentoyVersion();
    VentoyInstallOptions ventoyInstall;
    ventoyInstall.enableSecureBoot = gui_.VentoySecureBootChecked();
    ventoyInstall.useGpt = gui_.VentoyGptChecked();
    std::thread worker(&App::RunInstallThread, this, drive, format, runVentoy, pinVersion, ventoyInstall,
                       gui_.Hwnd());
    worker.detach();
}

namespace {

void PostVentoyStatus(const std::function<void(int, bool)>& postProgress, const std::wstring& key) {
    (void)key;
    postProgress(0, false);
}

bool ConfirmVentoy(HWND hwnd, const std::wstring& drive) {
    const int result = MessageBoxW(
        hwnd, i18n::Tr(L"ventoy_warning.message", drive).c_str(),
        i18n::Tr(L"ventoy_warning.title").c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    return result == IDYES;
}

bool ConfirmDriveLetterChange(HWND hwnd, const std::wstring& originalDrive, const std::wstring& newDrive) {
    const int result = MessageBoxW(
        hwnd, i18n::Tr(L"drive_letter_changed.message", originalDrive, newDrive).c_str(),
        i18n::Tr(L"drive_letter_changed.title").c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    return result == IDYES;
}

bool SameDriveLetter(const std::wstring& a, const std::wstring& b) {
    if (a.empty() || b.empty()) {
        return false;
    }
    wchar_t letterA = towupper(a[0]);
    wchar_t letterB = towupper(b[0]);
    return letterA == letterB;
}

bool ReconcileDriveLetter(HWND hwnd, std::wstring& drive, const DriveIdentity& identity,
                          const std::function<void(const std::wstring&)>& log,
                          const std::function<void()>& cancel, const std::function<void(const std::wstring&)>& fail,
                          const wchar_t* contextLabel) {
    const std::wstring resolved = ResolveDriveLetterAfterVentoy(drive, identity);
    if (resolved.empty()) {
        fail(i18n::Tr(L"messages.drive_lost_after_ventoy"));
        return false;
    }

    if (!SameDriveLetter(resolved, drive)) {
        log(std::wstring(contextLabel) + L": drive letter changed from " + drive + L" to " + resolved);
        if (!ConfirmDriveLetterChange(hwnd, drive, resolved)) {
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

void App::RunInstallThread(std::wstring drive, bool format, bool runVentoy, std::wstring pinVersion,
                            VentoyInstallOptions ventoyInstall, HWND hwnd) {
    const std::wstring root = root_;
    const std::wstring archive = ResolveMediCatArchivePath(root);
    const std::wstring sevenZip = sevenZa_;

    auto fail = [&](const std::wstring& msg) {
        log_->Error(msg);
        PostDone(false, msg);
    };

    auto cancel = [&] {
        log_->Info(L"Install cancelled by user");
        PostDone(false, L"");
    };

    const auto logInfo = [&](const std::wstring& msg) { log_->Info(msg); };

    const auto postProgress = [&](const int percent, const bool clearLog) {
        PostProgress(percent, clearLog);
    };

    std::wstring ventoyExe;
    DriveIdentity driveIdentity;
    if (runVentoy) {
        driveIdentity = GetDriveIdentity(drive);
        if (!driveIdentity.valid) {
            log_->Info(L"Could not read drive identity before Ventoy; letter remapping may be limited");
        }

        std::wstring netError;
        if (!CanInstallVentoyOffline(root, pinVersion) && !TestInternetConnection(netError)) {
            log_->Error(L"Internet check failed: " + netError);
            fail(i18n::Tr(L"messages.no_internet"));
            return;
        }

        VentoyEnsureOptions ensureOptions;
        ensureOptions.root = root;
        ensureOptions.sevenZipExe = sevenZip;
        ensureOptions.pinVersion = std::move(pinVersion);
        ensureOptions.onStatus = [&](const std::wstring& key) { PostVentoyStatus(postProgress, key); };
        ensureOptions.onLog = [&](const std::wstring& msg) { log_->Info(msg); };

        const VentoyResult ready = EnsureVentoyReady(ensureOptions);
        if (!ready.success) {
            fail(ready.error.empty() ? i18n::Tr(L"errors.ventoy_download_failed") : ready.error);
            return;
        }
        ventoyExe = ready.ventoyExe;
        log_->Info(L"Ventoy v" + ready.version + L" ready");

        PostProgress(0);
        const VentoyDetectionResult ventoyDetection = DetectVentoyOnDrive(drive);
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

        if (!ConfirmVentoy(hwnd, drive)) {
            cancel();
            return;
        }

        PostProgress(0);
        log_->Info(upgrade ? L"Running Ventoy upgrade" : L"Running Ventoy fresh install");
        log_->Info(L"Ventoy options: " + std::wstring(ventoyInstall.useGpt ? L"GPT" : L"MBR") + L", Secure Boot " +
                   (ventoyInstall.enableSecureBoot ? L"enabled" : L"disabled"));
        const VentoyResult ventoy = RunVentoyInstall(ventoyExe, drive, upgrade, ventoyInstall);
        if (!ventoy.success) {
            fail(upgrade ? i18n::Tr(L"messages.ventoy_upgrade_failed")
                         : i18n::Tr(L"messages.ventoy_install_failed"));
            return;
        }

        if (!ReconcileDriveLetter(hwnd, drive, driveIdentity, logInfo, cancel, fail, L"After Ventoy install")) {
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
        if (!FormatDriveNtfs(drive)) {
            log_->Debug(L"Format returned non-zero; continuing");
        }
    }

    if (runVentoy) {
        if (!ReconcileDriveLetter(hwnd, drive, driveIdentity, logInfo, cancel, fail,
                                  L"Final check before extract")) {
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

    PostExtractProgress(0, L"", true);

    const std::wstring extractLogPath = JoinPath(root_, L"extract.log");
    log_->Info(L"Writing raw 7za output to " + extractLogPath);

    const ExtractResult extract = Extract7zArchive(
        sevenZip, archive, dest, totalBytes, initialFree,
        [this](const ExtractProgress& p) { PostExtractProgress(p.percent, p.file, false); }, extractLogPath);

    if (!extract.success) {
        fail(i18n::Tr(L"messages.extraction_failed", extract.error));
        return;
    }

    PostProgress(0);
    const VerificationOutcome outcome = VerifyDriveFiles(drive);
    if (!outcome.success) {
        if (outcome.failedFiles > 0 && !outcome.failures.empty()) {
            if (PromptReExtract(outcome)) {
                const bool reextractOk = TryReExtractFailedFiles(drive, archive, outcome.failures);
                if (!reextractOk) {
                    fail(i18n::Tr(L"messages.re_extraction_error", L"selective extract failed"));
                    return;
                }

                PostProgress(0);
                const VerificationOutcome after = VerifyDriveFiles(drive, false);
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
