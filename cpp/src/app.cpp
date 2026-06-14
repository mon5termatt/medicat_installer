#include "app.h"

#include "bundle.h"
#include "drives.h"
#include "extract.h"
#include "util.h"
#include "ventoy.h"

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

std::wstring ShortPath(const std::wstring& path, size_t maxLen = 60) {
    if (path.size() <= maxLen) {
        return path;
    }
    return L"..." + path.substr(path.size() - (maxLen - 3));
}

}  // namespace

App::App(HINSTANCE instance) : instance_(instance) {
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
        MessageBoxW(gui_.Hwnd(), L"Failed to unpack bundled 7-Zip tools.", L"Startup error", MB_ICONERROR);
        return 1;
    }

    log_->Info(L"MediCat Installer (C++) started");

    gui_.SetInstallHandler([this] { OnInstall(); });
    return gui_.Run();
}

void App::PostProgress(const int percent, const std::wstring& status) {
    auto* payload = new ProgressPayload{percent, status};
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
        MessageBoxW(gui_.Hwnd(), L"Plug in a USB drive and select it from the list.", L"No drive",
                    MB_ICONWARNING);
        installing_ = false;
        return;
    }

    const std::wstring archive = JoinPath(root_, kMediCatArchiveName);
    if (!FileExists(archive)) {
        MessageBoxW(gui_.Hwnd(), L"MediCat archive not found beside the installer.", L"Missing file",
                    MB_ICONERROR);
        installing_ = false;
        return;
    }

    gui_.SetBusy(true);
    gui_.SetProgress(0, L"Starting... Do not unplug the USB.");
    log_->Info(L"Install started on " + drive);

    const bool format = gui_.FormatChecked();
    const bool skipVentoy = gui_.SkipVentoyChecked();
    std::thread worker(&App::RunInstallThread, this, drive, format, skipVentoy);
    worker.detach();
}

void App::RunInstallThread(std::wstring drive, bool format, bool skipVentoy) {
    const std::wstring root = root_;
    const std::wstring archive = JoinPath(root, kMediCatArchiveName);
    const std::wstring sevenZip = sevenZa_;
    const std::wstring ventoyExe = JoinPath(root, L"Ventoy2Disk\\Ventoy2Disk.exe");

    auto fail = [&](const std::wstring& msg) {
        log_->Error(msg);
        PostDone(false, msg);
    };

    if (!skipVentoy) {
        PostProgress(0, L"Installing Ventoy...");
        log_->Info(L"Running Ventoy install");
        const VentoyResult ventoy = RunVentoyInstall(ventoyExe, drive, false);
        if (!ventoy.success) {
            fail(L"Ventoy install failed.\n" + ventoy.error);
            return;
        }
    } else {
        log_->Info(L"Skipping Ventoy");
    }

    if (format) {
        PostProgress(0, L"Formatting USB...");
        log_->Info(L"Formatting " + drive);
        if (!FormatDriveNtfs(drive)) {
            log_->Debug(L"Format returned non-zero; continuing");
        }
    }

    std::wstring dest = drive;
    if (dest.size() == 2 && dest[1] == L':') {
        dest += L'\\';
    }

    PostProgress(0, L"Preparing extraction...");
    const uint64_t totalBytes = GetArchiveUncompressedSize(sevenZip, archive);
    const uint64_t initialFree = GetDriveFreeBytes(dest);
    log_->Debug(L"Uncompressed size: " + FormatBytes(totalBytes));
    log_->Debug(L"Initial free space: " + FormatBytes(initialFree));

    if (totalBytes > 0 && initialFree > 0 && totalBytes > initialFree) {
        fail(L"USB drive is too small for MediCat.");
        return;
    }

    PostProgress(0, L"Copying MediCat files... Do not unplug the USB.");

    const ExtractResult extract = Extract7zArchive(
        sevenZip, archive, dest, totalBytes, initialFree,
        [&](const ExtractProgress& p) {
            std::wostringstream status;
            status << L"Copying MediCat... " << p.percent << L"%";
            if (!p.file.empty()) {
                status << L"  " << ShortPath(p.file);
            } else if (p.bytesWritten > 0 && p.totalBytes > 0) {
                status << L"  (" << FormatBytes(p.bytesWritten) << L" / " << FormatBytes(p.totalBytes)
                       << L")";
            }
            PostProgress(p.percent, status.str());
        });

    if (!extract.success) {
        fail(L"Extraction failed.\n" + extract.error);
        return;
    }

    log_->Info(L"Install completed successfully");
    PostDone(true, L"MediCat is ready on " + drive + L".\nSafely remove the USB and boot from it.");
}

}  // namespace medicat
