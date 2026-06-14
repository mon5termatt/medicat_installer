#include "app.h"

#include "bundle.h"
#include "download.h"
#include "drives.h"
#include "extract.h"
#include "i18n.h"
#include "util.h"
#include "ventoy.h"

#include <functional>
#include <sstream>

namespace medicat {

constexpr wchar_t kMediCatArchiveName[] = L"MediCat.USB.v21.12.7z";

namespace {

void PostToGui(HWND hwnd, UINT msg, LPARAM payload) {
    if (hwnd) {
        PostMessageW(hwnd, msg, 0, payload);
    } else if (payload) {
        if (msg == WM_MEDICAT_PROGRESS) {
            delete reinterpret_cast<ProgressPayload*>(payload);
        } else if (msg == WM_MEDICAT_DONE) {
            delete reinterpret_cast<DonePayload*>(payload);
        }
    }
}

}  // namespace

App::App(HINSTANCE instance) : instance_(instance) {
    i18n::Load();
    root_ = GetExeDirectory();
    log_ = std::make_unique<Logger>(JoinPath(root_, L"medicat_installer.log"));

    const BundledTools tools = EnsureBundledTools(instance_);
    if (!tools.ok) {
        log_->Error(tools.error);
    } else {
        sevenZa_ = tools.sevenZa;
        sevenZ_ = tools.sevenZ;
        log_->Debug(L"Bundled 7za: " + sevenZa_);
        log_->Debug(L"Bundled 7z: " + sevenZ_);
    }
}

int App::Run() {
    if (!gui_.Create(instance_)) {
        return 1;
    }

    if (sevenZa_.empty() || sevenZ_.empty()) {
        MessageBoxW(gui_.Hwnd(), i18n::Tr(L"messages.7zip_not_found").c_str(),
                    i18n::Tr(L"titles.7zip_not_found").c_str(), MB_ICONERROR);
        return 1;
    }

    log_->Info(L"MediCat Installer (C++) started");

    gui_.SetInstallHandler([this] { OnInstall(); });
    return gui_.Run();
}

void App::PostProgress(const int percent, const bool clearLog) {
    auto* payload = new ProgressPayload{percent, clearLog};
    PostToGui(gui_.Hwnd(), WM_MEDICAT_PROGRESS, reinterpret_cast<LPARAM>(payload));
}

void App::PostDone(const bool success, const std::wstring& message) {
    installing_ = false;
    auto* payload = new DonePayload{success, message};
    PostToGui(gui_.Hwnd(), WM_MEDICAT_DONE, reinterpret_cast<LPARAM>(payload));
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

    const std::wstring archive = JoinPath(root_, kMediCatArchiveName);
    if (!FileExists(archive)) {
        MessageBoxW(gui_.Hwnd(),
                    i18n::Tr(L"messages.file_not_found", kMediCatArchiveName).c_str(),
                    i18n::Tr(L"titles.file_not_found").c_str(), MB_ICONERROR);
        installing_ = false;
        return;
    }

    gui_.SetBusy(true);
    gui_.ClearFileLog();
    gui_.SetProgress(0);
    log_->Info(L"Install started on " + drive);

    const bool format = gui_.FormatChecked();
    const bool skipVentoy = gui_.SkipVentoyChecked();
    const std::wstring pinVersion = gui_.PinnedVentoyVersion();
    std::thread worker(&App::RunInstallThread, this, drive, format, skipVentoy, pinVersion, gui_.Hwnd());
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

bool ConfirmVentoyMissing(HWND hwnd, const std::wstring& drive) {
    const int result = MessageBoxW(
        hwnd, i18n::Tr(L"ventoy_not_detected.message", drive).c_str(),
        i18n::Tr(L"ventoy_not_detected.title").c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    return result == IDYES;
}

}  // namespace

void App::RunInstallThread(std::wstring drive, bool format, bool skipVentoy, std::wstring pinVersion,
                            HWND hwnd) {
    const std::wstring root = root_;
    const std::wstring archive = JoinPath(root, kMediCatArchiveName);
    const std::wstring sevenZip = sevenZa_;

    auto fail = [&](const std::wstring& msg) {
        log_->Error(msg);
        PostDone(false, msg);
    };

    auto cancel = [&] {
        log_->Info(L"Install cancelled by user");
        PostDone(false, L"");
    };

    const auto postProgress = [&](const int percent, const bool clearLog) {
        PostProgress(percent, clearLog);
    };

    std::wstring ventoyExe;
    if (!skipVentoy) {
        std::wstring netError;
        if (!TestInternetConnection(netError)) {
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

        const bool upgrade = !format;
        if (upgrade) {
            PostProgress(0);
            log_->Info(L"Format disabled - checking for existing Ventoy");
            if (!TestVentoyInstalled(drive)) {
                log_->Info(i18n::Tr(L"log.ventoy_not_found"));
                if (!ConfirmVentoyMissing(hwnd, drive)) {
                    cancel();
                    return;
                }
            } else {
                log_->Info(i18n::Tr(L"log.ventoy_detected"));
            }
        } else {
            log_->Info(i18n::Tr(L"log.format_enabled"));
        }

        if (!ConfirmVentoy(hwnd, drive)) {
            cancel();
            return;
        }

        PostProgress(0);
        log_->Info(upgrade ? L"Running Ventoy upgrade" : L"Running Ventoy fresh install");
        const VentoyResult ventoy = RunVentoyInstall(ventoyExe, drive, upgrade);
        if (!ventoy.success) {
            fail(upgrade ? i18n::Tr(L"messages.ventoy_upgrade_failed")
                         : i18n::Tr(L"messages.ventoy_install_failed"));
            return;
        }
    } else {
        log_->Info(L"Skipping Ventoy");
    }

    if (format) {
        PostProgress(0);
        log_->Info(L"Formatting " + drive);
        if (!FormatDriveNtfs(drive)) {
            log_->Debug(L"Format returned non-zero; continuing");
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

    gui_.NotifyExtractProgress(0, L"", true);

    const std::wstring extractLogPath = JoinPath(root_, L"7za_extract.log");
    log_->Info(L"Writing raw 7za output to " + extractLogPath);

    const ExtractResult extract = Extract7zArchive(
        sevenZip, archive, dest, totalBytes, initialFree,
        [this](const ExtractProgress& p) { gui_.NotifyExtractProgress(p.percent, p.file); }, extractLogPath);

    if (!extract.success) {
        fail(i18n::Tr(L"messages.extraction_failed", extract.error));
        return;
    }

    log_->Info(L"Install completed successfully");
    PostDone(true, i18n::Tr(L"messages.installation_complete", drive));
}

}  // namespace medicat
