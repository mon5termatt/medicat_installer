#include "gui.h"

#include "drives.h"

#include <commctrl.h>

#include <sstream>

namespace medicat {

namespace {

constexpr int kDriveComboId = 1001;
constexpr int kFormatCheckId = 1002;
constexpr int kSkipVentoyCheckId = 1003;
constexpr int kInstallBtnId = 1004;
constexpr int kProgressId = 1005;
constexpr int kStatusId = 1006;

HFONT MakeUiFont() {
    return CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

}  // namespace

bool Gui::Create(HINSTANCE instance) {
    instance_ = instance;

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    const wchar_t* cls = L"MedicatInstallerWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = Gui::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = cls;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        0, cls, L"MediCat USB Installer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 560, 320,
        nullptr, nullptr, instance, this);

    return hwnd_ != nullptr;
}

int Gui::Run() {
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void Gui::SetInstallHandler(InstallHandler handler) { onInstall_ = std::move(handler); }

void Gui::SetBusy(const bool busy) {
    EnableWindow(installBtn_, !busy);
    EnableWindow(driveCombo_, !busy);
    EnableWindow(formatCheck_, !busy);
    EnableWindow(skipVentoyCheck_, !busy);
}

void Gui::SetProgress(const int percent, const std::wstring& status) {
    SendMessageW(progressBar_, PBM_SETPOS, percent, 0);
    SetWindowTextW(statusLabel_, status.c_str());
}

void Gui::ShowDone(const bool success, const std::wstring& message) {
    SetBusy(false);
    SetProgress(success ? 100 : 0, message);
    MessageBoxW(hwnd_, message.c_str(), success ? L"Done" : L"Error",
                success ? MB_ICONINFORMATION : MB_ICONERROR);
}

std::wstring Gui::SelectedDrive() const {
    const int idx = static_cast<int>(SendMessageW(driveCombo_, CB_GETCURSEL, 0, 0));
    if (idx < 0) {
        return L"";
    }
    const auto* letter = reinterpret_cast<std::wstring*>(SendMessageW(driveCombo_, CB_GETITEMDATA, idx, 0));
    if (letter) {
        return *letter;
    }
    return L"";
}

bool Gui::FormatChecked() const {
    return SendMessageW(formatCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool Gui::SkipVentoyChecked() const {
    return SendMessageW(skipVentoyCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

LRESULT CALLBACK Gui::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    Gui* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<Gui*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<Gui*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
        case WM_CREATE:
            self->OnCreate(hwnd);
            return 0;
        case WM_COMMAND:
            self->OnCommand(wp);
            return 0;
        case WM_MEDICAT_PROGRESS: {
            auto* payload = reinterpret_cast<ProgressPayload*>(lp);
            if (payload) {
                self->SetProgress(payload->percent, payload->status);
                delete payload;
            }
            return 0;
        }
        case WM_MEDICAT_DONE: {
            auto* payload = reinterpret_cast<DonePayload*>(lp);
            if (payload) {
                self->ShowDone(payload->success, payload->message);
                delete payload;
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void Gui::OnCreate(HWND hwnd) {
    hwnd_ = hwnd;
    const HFONT font = MakeUiFont();

    CreateWindowW(L"STATIC", L"USB drive:", WS_CHILD | WS_VISIBLE, 20, 20, 100, 22, hwnd, nullptr,
                  instance_, nullptr);
    driveCombo_ = CreateWindowW(
        WC_COMBOBOXW, nullptr,
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        120, 18, 400, 300, hwnd, reinterpret_cast<HMENU>(kDriveComboId), instance_, nullptr);

    formatCheck_ = CreateWindowW(
        L"BUTTON", L"Erase everything on this USB (format)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        20, 60, 500, 24, hwnd, reinterpret_cast<HMENU>(kFormatCheckId), instance_, nullptr);
    SendMessageW(formatCheck_, BM_SETCHECK, BST_CHECKED, 0);

    skipVentoyCheck_ = CreateWindowW(
        L"BUTTON", L"Skip Ventoy (USB already has Ventoy)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        20, 90, 500, 24, hwnd, reinterpret_cast<HMENU>(kSkipVentoyCheckId), instance_, nullptr);

    installBtn_ = CreateWindowW(
        L"BUTTON", L"INSTALL MEDICAT",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        20, 130, 500, 40, hwnd, reinterpret_cast<HMENU>(kInstallBtnId), instance_, nullptr);

    progressBar_ = CreateWindowW(
        PROGRESS_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        20, 190, 500, 24, hwnd, reinterpret_cast<HMENU>(kProgressId), instance_, nullptr);
    SendMessageW(progressBar_, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    statusLabel_ = CreateWindowW(
        L"STATIC", L"Plug in your USB and click Install.",
        WS_CHILD | WS_VISIBLE,
        20, 225, 500, 40, hwnd, reinterpret_cast<HMENU>(kStatusId), instance_, nullptr);

    for (HWND child : {driveCombo_, formatCheck_, skipVentoyCheck_, installBtn_, statusLabel_}) {
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    RefreshDrives();
}

void Gui::OnCommand(WPARAM wp) {
    const int id = LOWORD(wp);
    if (id == kInstallBtnId && onInstall_) {
        onInstall_();
    }
}

void Gui::RefreshDrives() {
    SendMessageW(driveCombo_, CB_RESETCONTENT, 0, 0);
    const auto drives = ListTargetDrives();
    for (const auto& d : drives) {
        SendMessageW(driveCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(d.display.c_str()));
        const int idx = static_cast<int>(SendMessageW(driveCombo_, CB_GETCOUNT, 0, 0)) - 1;
        SendMessageW(driveCombo_, CB_SETITEMDATA, idx, reinterpret_cast<LPARAM>(new std::wstring(d.letter)));
    }
    const int def = DefaultDriveIndex(drives);
    if (def >= 0) {
        SendMessageW(driveCombo_, CB_SETCURSEL, def, 0);
    }
}

}  // namespace medicat
