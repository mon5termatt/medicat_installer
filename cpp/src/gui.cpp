#include "gui.h"

#include "drives.h"
#include "i18n.h"
#include "offline.h"
#include "resource.h"
#include "theme.h"
#include "util.h"
#include "ventoy.h"

#include <commctrl.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>
#include <sstream>
#include <thread>

namespace medicat {

namespace {

#ifndef INSTALLER_VERSION
#define INSTALLER_VERSION "dev"
#endif

constexpr int kMargin = 20;
constexpr int kContentWidth = 520;
constexpr int kWindowWidth = 560;
constexpr int kHeaderHeight = 84;
constexpr int kLogoMaxSize = 48;
constexpr int kHeaderLogoX = kMargin;
constexpr int kHeaderLogoY = (kHeaderHeight - kLogoMaxSize) / 2;
constexpr int kHeaderTitleX = kHeaderLogoX + kLogoMaxSize + 10;
constexpr int kHeaderTitleY = kHeaderLogoY + 10;
constexpr int kLanguageComboWidth = 148;
constexpr int kLanguageComboHeight = 24;
constexpr int kLanguageComboInset = 24;
constexpr int kLanguageComboX = kMargin + kContentWidth - kLanguageComboWidth - kLanguageComboInset;
constexpr int kLanguageComboY = kHeaderTitleY;
constexpr int kHeaderTitleWidth = kLanguageComboX - kHeaderTitleX - 12;
constexpr int kVersionLabelHeight = 18;
constexpr int kVersionBottomMargin = 10;
constexpr int kContentTop = kHeaderHeight + 10;

constexpr int kCheckboxHeight = 24;
constexpr int kSectionGap = 12;
constexpr int kInstallBtnHeight = 44;
constexpr int kInstallBtnWidth = 252;
constexpr int kVerifyBtnWidth = kContentWidth - kInstallBtnWidth - 10;
constexpr int kVerifyBtnX = kMargin + kInstallBtnWidth + 10;
constexpr int kProgressHeight = 28;
constexpr int kOpenLogBtnHeight = 32;
constexpr int kActionRowGap = 12;
constexpr int kCurrentFileHeight = 20;
constexpr int kBottomChrome = kVersionBottomMargin + kVersionLabelHeight + 8;

constexpr int kDriveComboY = kContentTop + 22;
constexpr int kShowAllDrivesY = kDriveComboY + 32;
constexpr int kFormatY = kShowAllDrivesY + 28;
constexpr int kSkipVentoyY = kFormatY + 30;
constexpr int kAdvancedY = kSkipVentoyY + 30;
constexpr int kPinVentoyY = kAdvancedY + 28;
constexpr int kVentoyComboY = kPinVentoyY - 2;
constexpr int kVentoySecureBootY = kPinVentoyY + 28;
constexpr int kGptY = kVentoySecureBootY + 30;
constexpr int kOptionsBottomCollapsed = kAdvancedY + kCheckboxHeight;
constexpr int kOptionsBottomExpanded = kGptY + kCheckboxHeight;

constexpr int kInstallYCollapsed = kOptionsBottomCollapsed + kSectionGap;
constexpr int kInstallYExpanded = kOptionsBottomExpanded + kSectionGap;
constexpr int kProgressYCollapsed = kInstallYCollapsed + kInstallBtnHeight + kActionRowGap;
constexpr int kProgressYExpanded = kInstallYExpanded + kInstallBtnHeight + kActionRowGap;
constexpr int kOpenLogYCollapsed = kProgressYCollapsed + (kProgressHeight - kOpenLogBtnHeight) / 2;
constexpr int kOpenLogYExpanded = kProgressYExpanded + (kProgressHeight - kOpenLogBtnHeight) / 2;
constexpr int kCurrentFileYCollapsed = kProgressYCollapsed + kOpenLogBtnHeight + 8;
constexpr int kCurrentFileYExpanded = kProgressYExpanded + kOpenLogBtnHeight + 8;

constexpr int kWindowHeightCollapsed = kCurrentFileYCollapsed + kCurrentFileHeight + kBottomChrome + 16;
constexpr int kWindowHeightExpanded = kCurrentFileYExpanded + kCurrentFileHeight + kBottomChrome + 16;

constexpr int kDriveComboId = 1001;
constexpr int kLanguageComboId = 1018;
constexpr int kShowAllDrivesCheckId = 1016;
constexpr int kFormatCheckId = 1002;
constexpr int kSkipVentoyCheckId = 1003;
constexpr int kInstallBtnId = 1004;
constexpr int kProgressId = 1005;
constexpr int kStatusId = 1006;
constexpr int kAdvancedCheckId = 1007;
constexpr int kPinVentoyCheckId = 1008;
constexpr int kVentoyVersionEditId = 1009;
constexpr int kOpenLogBtnId = 1010;
constexpr int kFileLogListId = 1011;
constexpr int kCurrentFileLabelId = 1012;
constexpr int kVentoySecureBootCheckId = 1013;
constexpr int kVentoyGptCheckId = 1014;
constexpr int kVerifyFilesBtnId = 1015;
constexpr UINT_PTR kUiRefreshTimerId = 1;
constexpr UINT kUiRefreshIntervalMs = 250;
constexpr wchar_t kFileLogWindowClass[] = L"MedicatFileLogWindow";

struct LanguageOption {
    const wchar_t* code;
    const wchar_t* nativeName;
};

constexpr LanguageOption kLanguageOptions[] = {
    {L"en", L"English"},
    {L"es", L"Español"},
    {L"fr", L"Français"},
    {L"pl", L"Polski"},
    {L"tr", L"Türkçe"},
};

int LanguageIndexForCode(const std::wstring& code) {
    for (size_t i = 0; i < std::size(kLanguageOptions); ++i) {
        if (code == kLanguageOptions[i].code) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

const wchar_t* LanguageCodeForIndex(const int index) {
    if (index < 0 || index >= static_cast<int>(std::size(kLanguageOptions))) {
        return kLanguageOptions[0].code;
    }
    return kLanguageOptions[index].code;
}

struct GlowButtonState {
    WNDPROC original = nullptr;
    bool hovered = false;
    bool pressed = false;
    bool primary = false;
};

LRESULT CALLBACK GlowButtonProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
    auto* state = reinterpret_cast<GlowButtonState*>(GetPropW(hwnd, L"MedicatGlowBtn"));
    if (!state) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
        case WM_MOUSEMOVE:
            if (!state->hovered) {
                state->hovered = true;
                TRACKMOUSEEVENT tme{};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        case WM_MOUSELEAVE:
            state->hovered = false;
            state->pressed = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case WM_LBUTTONDOWN:
            if (IsWindowEnabled(hwnd)) {
                state->pressed = true;
                SetCapture(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONUP: {
            const bool wasPressed = state->pressed;
            state->pressed = false;
            if (GetCapture() == hwnd) {
                ReleaseCapture();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            if (wasPressed && IsWindowEnabled(hwnd)) {
                POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                RECT rc{};
                GetClientRect(hwnd, &rc);
                if (PtInRect(&rc, pt)) {
                    const int id = GetDlgCtrlID(hwnd);
                    SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, BN_CLICKED),
                                 reinterpret_cast<LPARAM>(hwnd));
                }
            }
            return 0;
        }
        case WM_KEYDOWN:
            if ((wp == VK_SPACE || wp == VK_RETURN) && IsWindowEnabled(hwnd)) {
                state->pressed = true;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            break;
        case WM_KEYUP:
            if ((wp == VK_SPACE || wp == VK_RETURN) && state->pressed) {
                state->pressed = false;
                InvalidateRect(hwnd, nullptr, FALSE);
                if (IsWindowEnabled(hwnd)) {
                    const int id = GetDlgCtrlID(hwnd);
                    SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, BN_CLICKED),
                                 reinterpret_cast<LPARAM>(hwnd));
                }
                return 0;
            }
            break;
        case WM_ENABLE:
            state->hovered = false;
            state->pressed = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_UPDATEUISTATE:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case BM_SETSTATE:
            return 0;
        case WM_PRINTCLIENT: {
            const HDC hdc = reinterpret_cast<HDC>(wp);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            wchar_t text[256]{};
            GetWindowTextW(hwnd, text, static_cast<int>(std::size(text)));
            const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));

            theme::ButtonState btnState = theme::ButtonState::Normal;
            if (!IsWindowEnabled(hwnd)) {
                btnState = theme::ButtonState::Disabled;
            } else if (state->pressed) {
                btnState = theme::ButtonState::Pressed;
            } else if (state->hovered) {
                btnState = theme::ButtonState::Hovered;
            }

            theme::PaintFlatButton(
                hdc, rc, text, font,
                state->primary ? theme::ButtonStyle::Primary : theme::ButtonStyle::Secondary, btnState);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            const HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            wchar_t text[256]{};
            GetWindowTextW(hwnd, text, static_cast<int>(std::size(text)));
            const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));

            theme::ButtonState btnState = theme::ButtonState::Normal;
            if (!IsWindowEnabled(hwnd)) {
                btnState = theme::ButtonState::Disabled;
            } else if (state->pressed) {
                btnState = theme::ButtonState::Pressed;
            } else if (state->hovered) {
                btnState = theme::ButtonState::Hovered;
            }

            theme::PaintFlatButton(
                hdc, rc, text, font,
                state->primary ? theme::ButtonStyle::Primary : theme::ButtonStyle::Secondary, btnState);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_NCDESTROY:
            RemovePropW(hwnd, L"MedicatGlowBtn");
            delete state;
            break;
        default:
            break;
    }

    return CallWindowProcW(state->original, hwnd, msg, wp, lp);
}

void SubclassGlowButton(HWND hwnd, const bool primary) {
    SetWindowTheme(hwnd, L"", L"");
    SendMessageW(hwnd, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    auto* state = new GlowButtonState{};
    state->primary = primary;
    state->original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GlowButtonProc)));
    SetPropW(hwnd, L"MedicatGlowBtn", state);
}

struct FlatCheckboxState {
    WNDPROC original = nullptr;
};

void PaintFlatCheckboxControl(const HWND hwnd, const HDC hdc) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    wchar_t text[256]{};
    GetWindowTextW(hwnd, text, static_cast<int>(std::size(text)));
    const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    const bool checked = SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    theme::PaintFlatCheckbox(hdc, rc, text, font, checked, IsWindowEnabled(hwnd) != FALSE);
}

void ToggleFlatCheckbox(const HWND hwnd) {
    const bool checked = SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    SendMessageW(hwnd, BM_SETCHECK, checked ? BST_UNCHECKED : BST_CHECKED, 0);
    InvalidateRect(hwnd, nullptr, FALSE);
    const int id = GetDlgCtrlID(hwnd);
    SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), reinterpret_cast<LPARAM>(hwnd));
}

LRESULT CALLBACK FlatCheckboxProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
    auto* state = reinterpret_cast<FlatCheckboxState*>(GetPropW(hwnd, L"MedicatFlatCheck"));
    if (!state) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
        case BM_GETCHECK:
        case BM_SETCHECK: {
            const LRESULT result = CallWindowProcW(state->original, hwnd, msg, wp, lp);
            if (msg == BM_SETCHECK) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return result;
        }
        case WM_LBUTTONDOWN:
            if (IsWindowEnabled(hwnd)) {
                ToggleFlatCheckbox(hwnd);
            }
            return 0;
        case WM_KEYUP:
            if (wp == VK_SPACE && IsWindowEnabled(hwnd)) {
                ToggleFlatCheckbox(hwnd);
                return 0;
            }
            break;
        case WM_ENABLE:
        case WM_UPDATEUISTATE:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case BM_SETSTATE:
            return 0;
        case WM_PRINTCLIENT: {
            PaintFlatCheckboxControl(hwnd, reinterpret_cast<HDC>(wp));
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            const HDC hdc = BeginPaint(hwnd, &ps);
            PaintFlatCheckboxControl(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_NCDESTROY:
            RemovePropW(hwnd, L"MedicatFlatCheck");
            delete state;
            break;
        default:
            break;
    }

    return CallWindowProcW(state->original, hwnd, msg, wp, lp);
}

void SubclassFlatCheckbox(const HWND hwnd) {
    SetWindowTheme(hwnd, L"", L"");
    auto* state = new FlatCheckboxState{};
    state->original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FlatCheckboxProc)));
    SetPropW(hwnd, L"MedicatFlatCheck", state);
}

}  // namespace

LRESULT CALLBACK Gui::ProgressBarProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
    const auto original = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps{};
        const HDC hdc = BeginPaint(hwnd, &ps);
        const auto* gui = reinterpret_cast<Gui*>(GetPropW(hwnd, L"MedicatGui"));
        if (gui) {
            RECT rc{};
            GetClientRect(hwnd, &rc);
            const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
            theme::PaintProgressBar(hdc, rc, gui->progressPercentValue_, gui->progressPercentText_, font);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        return 1;
    }
    return CallWindowProcW(original, hwnd, msg, wp, lp);
}

bool Gui::Create(HINSTANCE instance) {
    instance_ = instance;
    theme::Initialize();

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
    wc.hbrBackground = theme::GetBrushes().window;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = wc.hIcon;
    wc.lpszClassName = cls;
    RegisterClassExW(&wc);

    WNDCLASSEXW logWc{};
    logWc.cbSize = sizeof(logWc);
    logWc.lpfnWndProc = Gui::FileLogWndProc;
    logWc.hInstance = instance;
    logWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    logWc.hbrBackground = theme::GetBrushes().window;
    logWc.lpszClassName = kFileLogWindowClass;
    RegisterClassExW(&logWc);

    hwnd_ = CreateWindowExW(
        0, cls, i18n::Tr(L"ui.title_label").c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, kWindowWidth, kWindowHeightCollapsed,
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

void Gui::SetVerifyHandler(InstallHandler handler) { onVerify_ = std::move(handler); }

void Gui::SetBusy(const bool busy, const BusyProgressMode progressMode) {
    EnableWindow(installBtn_, !busy);
    EnableWindow(verifyFilesBtn_, !busy);
    EnableWindow(driveCombo_, !busy);
    EnableWindow(languageCombo_, !busy);
    EnableWindow(showAllDrivesCheck_, !busy);
    EnableWindow(formatCheck_, !busy);
    EnableWindow(skipVentoyCheck_, !busy);
    EnableWindow(advancedCheck_, !busy);
    UpdateAdvancedControls();
    if (busy) {
        busyProgressMode_ = progressMode;
        EnableWindow(pinVentoyCheck_, FALSE);
        EnableWindow(ventoyVersionCombo_, FALSE);
        if (progressMode == BusyProgressMode::FileLog || progressMode == BusyProgressMode::Verify) {
            SetTimer(hwnd_, kUiRefreshTimerId, kUiRefreshIntervalMs, nullptr);
        }
    } else {
        if (busyProgressMode_ == BusyProgressMode::FileLog || busyProgressMode_ == BusyProgressMode::Verify) {
            KillTimer(hwnd_, kUiRefreshTimerId);
            FlushInstallUi();
        }
        busyProgressMode_ = BusyProgressMode::None;
    }
}

void Gui::NotifyExtractProgress(const int percent, const std::wstring& file, const bool resetLog) {
    std::lock_guard lock(uiMutex_);
    if (resetLog) {
        fileLogLines_.clear();
        pendingFileLines_.clear();
        fileLogDisplayLines_.clear();
        pendingPercent_ = 0;
        pendingResetLog_ = true;
        return;
    }

    pendingPercent_ = percent;
    if (!file.empty() && (fileLogLines_.empty() || fileLogLines_.back() != file)) {
        fileLogLines_.push_back(file);
        pendingFileLines_.push_back(file);
    }
}

void Gui::FlushInstallUi() {
    int percent = 0;
    bool reset = false;
    std::vector<std::wstring> newFiles;
    size_t startIndex = 0;

    {
        std::lock_guard lock(uiMutex_);
        percent = pendingPercent_;
        reset = pendingResetLog_;
        pendingResetLog_ = false;
        newFiles.swap(pendingFileLines_);
        if (!newFiles.empty()) {
            startIndex = fileLogLines_.size() - newFiles.size() + 1;
        }
    }

    if (reset) {
        ClearFileLog();
    }

    progressPercentValue_ = percent;
    progressPercentText_ = std::to_wstring(percent) + L"%";
    InvalidateRect(progressBar_, nullptr, FALSE);

    if (!newFiles.empty()) {
        SetWindowTextW(currentFileLabel_, ShortDisplayPath(newFiles.back()).c_str());
        if (fileLogList_) {
            BatchAppendDetailLog(newFiles, startIndex);
        }
    }
}

void Gui::SetProgress(const int percent, const bool clearLog) {
    if (clearLog) {
        ClearFileLog();
    }
    progressPercentValue_ = percent;
    progressPercentText_ = std::to_wstring(percent) + L"%";
    InvalidateRect(progressBar_, nullptr, FALSE);
}

void Gui::SetCurrentFileLabel(const std::wstring& text) {
    SetWindowTextW(currentFileLabel_, ShortDisplayPath(text).c_str());
}

void Gui::ClearFileLog() {
    {
        std::lock_guard lock(uiMutex_);
        fileLogLines_.clear();
        fileLogDisplayLines_.clear();
    }
    SetWindowTextW(currentFileLabel_, L"");
    if (fileLogList_) {
        SendMessageW(fileLogList_, LB_RESETCONTENT, 0, 0);
    }
}

std::wstring Gui::FormatLogLine(const size_t index, const std::wstring& path) const {
    wchar_t prefix[16]{};
    swprintf_s(prefix, L"%5zu  ", index);
    return std::wstring(prefix) + path;
}

void Gui::BatchAppendDetailLog(const std::vector<std::wstring>& files, const size_t startIndex) {
    if (!fileLogList_ || files.empty()) {
        return;
    }

    for (size_t i = 0; i < files.size(); ++i) {
        fileLogDisplayLines_.push_back(FormatLogLine(startIndex + i, files[i]));
        SendMessageW(fileLogList_, LB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(fileLogDisplayLines_.back().c_str()));
    }

    const int count = static_cast<int>(SendMessageW(fileLogList_, LB_GETCOUNT, 0, 0));
    if (count > 0) {
        SendMessageW(fileLogList_, LB_SETTOPINDEX, count - 1, 0);
    }
}

void Gui::SyncDetailLog() {
    if (!fileLogList_) {
        return;
    }

    fileLogDisplayLines_.clear();
    SendMessageW(fileLogList_, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < fileLogLines_.size(); ++i) {
        fileLogDisplayLines_.push_back(FormatLogLine(i + 1, fileLogLines_[i]));
        SendMessageW(fileLogList_, LB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(fileLogDisplayLines_.back().c_str()));
    }

    const int count = static_cast<int>(SendMessageW(fileLogList_, LB_GETCOUNT, 0, 0));
    if (count > 0) {
        SendMessageW(fileLogList_, LB_SETTOPINDEX, count - 1, 0);
    }
}

void Gui::ResizeFileLogWindow(const HWND hwnd) {
    if (!fileLogList_) {
        return;
    }
    RECT rc{};
    GetClientRect(hwnd, &rc);
    SetWindowPos(fileLogList_, nullptr, 8, 8, rc.right - 16, rc.bottom - 16, SWP_NOZORDER);
}

void Gui::OpenFileLogWindow() {
    if (fileLogWindow_ && IsWindow(fileLogWindow_)) {
        ShowWindow(fileLogWindow_, SW_SHOW);
        SetForegroundWindow(fileLogWindow_);
        return;
    }

    RECT mainRc{};
    GetWindowRect(hwnd_, &mainRc);

    fileLogWindow_ = CreateWindowExW(
        0, kFileLogWindowClass, i18n::Tr(L"ui.file_log_title").c_str(),
        WS_OVERLAPPEDWINDOW, mainRc.right + 12, mainRc.top, 720, 480, hwnd_, nullptr, instance_,
        this);
    if (!fileLogWindow_) {
        return;
    }

    theme::EnableDarkModeRecursive(fileLogWindow_);

    const HFONT logFont = theme::MakeLogFont();
    fileLogList_ = CreateWindowW(
        L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
        8, 8, 680, 420, fileLogWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kFileLogListId)), instance_, nullptr);
    SendMessageW(fileLogList_, WM_SETFONT, reinterpret_cast<WPARAM>(logFont), TRUE);

    SyncDetailLog();
    SendMessageW(fileLogList_, LB_SETHORIZONTALEXTENT, 8000, 0);
    ResizeFileLogWindow(fileLogWindow_);
    ShowWindow(fileLogWindow_, SW_SHOW);
    UpdateWindow(fileLogWindow_);
}

LRESULT CALLBACK Gui::FileLogWndProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
    Gui* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<Gui*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }

    self = reinterpret_cast<Gui*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
        case WM_SIZE:
            self->ResizeFileLogWindow(hwnd);
            return 0;
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, theme::GetBrushes().window);
            return 1;
        }
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, theme::Colors().control);
            SetTextColor(hdc, theme::Colors().text);
            return reinterpret_cast<LRESULT>(theme::GetBrushes().control);
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            self->fileLogWindow_ = nullptr;
            self->fileLogList_ = nullptr;
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void Gui::ShowDone(const bool success, const std::wstring& message, const std::wstring& title) {
    SetBusy(false);
    if (!success && message.empty()) {
        SetProgress(0);
        return;
    }
    SetProgress(success ? 100 : 0);
    const std::wstring dialogTitle =
        title.empty() ? (success ? i18n::Tr(L"titles.installation_complete") : i18n::Tr(L"titles.installation_error"))
                      : title;
    MessageBoxW(hwnd_, message.c_str(), dialogTitle.c_str(), success ? MB_ICONINFORMATION : MB_ICONERROR);
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

bool Gui::AdvancedChecked() const {
    return SendMessageW(advancedCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool Gui::PinVentoyVersionChecked() const {
    return AdvancedChecked() &&
           SendMessageW(pinVentoyCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool Gui::VentoySecureBootChecked() const {
    return AdvancedChecked() &&
           SendMessageW(ventoySecureBootCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool Gui::VentoyGptChecked() const {
    return AdvancedChecked() &&
           SendMessageW(ventoyGptCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool Gui::ShowAllDrivesChecked() const {
    return SendMessageW(showAllDrivesCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

std::wstring Gui::PinnedVentoyVersion() const {
    if (!PinVentoyVersionChecked()) {
        return L"";
    }

    const int idx = static_cast<int>(SendMessageW(ventoyVersionCombo_, CB_GETCURSEL, 0, 0));
    if (idx < 0 || static_cast<size_t>(idx) >= ventoyVersions_.size()) {
        return L"";
    }
    return ventoyVersions_[static_cast<size_t>(idx)];
}

void Gui::EnsureVentoyVersionsLoaded() {
    if (ventoyVersionsLoading_) {
        return;
    }

    if (!ventoyVersionsLoaded_) {
        if (LoadOfflineVentoyVersionList(ventoyVersions_)) {
            PopulateVentoyVersionCombo();
            ventoyVersionsLoaded_ = true;
        } else if (LoadBundledVentoyVersionList(instance_, ventoyVersions_)) {
            PopulateVentoyVersionCombo();
            ventoyVersionsLoaded_ = true;
        }
    }

    ventoyVersionsLoading_ = true;
    std::thread([hwnd = hwnd_]() {
        std::vector<std::wstring> versions;
        const VentoyResult fetched = FetchVentoyVersions(versions);
        LPARAM payload = 0;
        if (fetched.success && !versions.empty()) {
            payload = reinterpret_cast<LPARAM>(new std::vector<std::wstring>(std::move(versions)));
        }
        PostMessageW(hwnd, WM_MEDICAT_VENTOY_VERSIONS, 0, payload);
    }).detach();
}

void Gui::SetVentoyVersions(std::vector<std::wstring> versions) {
    ventoyVersionsLoading_ = false;
    ventoyVersionsLoaded_ = true;
    ventoyVersions_ = std::move(versions);
    PopulateVentoyVersionCombo();
}

void Gui::PopulateVentoyVersionCombo() {
    if (!ventoyVersionCombo_) {
        return;
    }

    SendMessageW(ventoyVersionCombo_, CB_RESETCONTENT, 0, 0);
    for (const auto& version : ventoyVersions_) {
        const std::wstring display = L"v" + version;
        SendMessageW(ventoyVersionCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display.c_str()));
    }

    if (!ventoyVersions_.empty()) {
        SendMessageW(ventoyVersionCombo_, CB_SETCURSEL, 0, 0);
    }
}

void Gui::LayoutVersionLabel() {
    if (!versionLabel_) {
        return;
    }

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const int y = rc.bottom - kVersionBottomMargin - kVersionLabelHeight;
    SetWindowPos(versionLabel_, nullptr, kMargin, y, rc.right - 2 * kMargin, kVersionLabelHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void Gui::LayoutHeader() {
    if (logoStatic_ && IsWindow(logoStatic_)) {
        SetWindowPos(logoStatic_, nullptr, kHeaderLogoX, kHeaderLogoY, kLogoMaxSize, kLogoMaxSize,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (titleLabel_ && IsWindow(titleLabel_)) {
        SetWindowPos(titleLabel_, nullptr, kHeaderTitleX, kHeaderTitleY, kHeaderTitleWidth, 28,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (languageCombo_ && IsWindow(languageCombo_)) {
        SetWindowPos(languageCombo_, nullptr, kLanguageComboX, kLanguageComboY, kLanguageComboWidth,
                     kLanguageComboHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void Gui::UpdateAdvancedControls() {
    const bool expanded = AdvancedChecked();
    const bool pin = expanded && PinVentoyVersionChecked();
    const bool interactive = IsWindowEnabled(advancedCheck_);

    ShowWindow(pinVentoyCheck_, expanded ? SW_SHOW : SW_HIDE);
    ShowWindow(ventoySecureBootCheck_, expanded ? SW_SHOW : SW_HIDE);
    ShowWindow(ventoyGptCheck_, expanded ? SW_SHOW : SW_HIDE);
    ShowWindow(ventoyVersionCombo_, pin ? SW_SHOW : SW_HIDE);
    EnableWindow(pinVentoyCheck_, expanded && interactive);
    EnableWindow(ventoySecureBootCheck_, expanded && interactive);
    EnableWindow(ventoyGptCheck_, expanded && interactive);
    EnableWindow(ventoyVersionCombo_, pin && interactive);

    if (expanded) {
        EnsureVentoyVersionsLoaded();
    }

    const int installY = expanded ? kInstallYExpanded : kInstallYCollapsed;
    const int progressY = expanded ? kProgressYExpanded : kProgressYCollapsed;
    const int openLogY = expanded ? kOpenLogYExpanded : kOpenLogYCollapsed;
    const int currentFileY = expanded ? kCurrentFileYExpanded : kCurrentFileYCollapsed;

    SetWindowPos(installBtn_, nullptr, kMargin, installY, kInstallBtnWidth, kInstallBtnHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(verifyFilesBtn_, nullptr, kVerifyBtnX, installY, kVerifyBtnWidth, kInstallBtnHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(progressBar_, nullptr, kMargin, progressY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(openLogBtn_, nullptr, 390, openLogY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(currentFileLabel_, nullptr, kMargin, currentFileY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    RECT wr{};
    GetWindowRect(hwnd_, &wr);
    const int targetH = expanded ? kWindowHeightExpanded : kWindowHeightCollapsed;
    if ((wr.bottom - wr.top) != targetH) {
        SetWindowPos(hwnd_, nullptr, 0, 0, kWindowWidth, targetH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hwnd_, nullptr, TRUE);
    }

    LayoutVersionLabel();
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
                if (payload->extractUpdate) {
                    self->NotifyExtractProgress(payload->percent, payload->file, payload->resetLog);
                } else {
                    self->SetProgress(payload->percent, payload->clearLog);
                }
                delete payload;
            }
            return 0;
        }
        case WM_TIMER:
            if (wp == kUiRefreshTimerId) {
                self->FlushInstallUi();
            }
            return 0;
        case WM_SIZE:
            self->LayoutHeader();
            self->LayoutVersionLabel();
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            const HDC hdc = BeginPaint(hwnd, &ps);
            RECT logoRect{kHeaderLogoX, kHeaderLogoY, kHeaderLogoX + kLogoMaxSize,
                          kHeaderLogoY + kLogoMaxSize};
            theme::PaintLogo(hdc, self->instance_, logoRect, kLogoMaxSize);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, theme::GetBrushes().window);
            const HPEN pen = CreatePen(PS_SOLID, 1, theme::Colors().border);
            const HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
            MoveToEx(hdc, 0, kHeaderHeight - 1, nullptr);
            LineTo(hdc, rc.right, kHeaderHeight - 1);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
            return 1;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            const HWND ctl = reinterpret_cast<HWND>(lp);
            SetBkMode(hdc, TRANSPARENT);
            if (ctl == self->titleLabel_) {
                SetTextColor(hdc, theme::Colors().text);
                return reinterpret_cast<LRESULT>(theme::GetBrushes().window);
            }
            if (ctl == self->versionLabel_ || ctl == self->currentFileLabel_) {
                SetTextColor(hdc, theme::Colors().muted);
                return reinterpret_cast<LRESULT>(theme::GetBrushes().window);
            }
            SetTextColor(hdc, theme::Colors().text);
            return reinterpret_cast<LRESULT>(theme::GetBrushes().window);
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, theme::Colors().control);
            SetTextColor(hdc, theme::Colors().text);
            return reinterpret_cast<LRESULT>(theme::GetBrushes().control);
        }
        case WM_MEDICAT_DONE: {
            auto* payload = reinterpret_cast<DonePayload*>(lp);
            if (payload) {
                self->ShowDone(payload->success, payload->message, payload->title);
                delete payload;
            }
            return 0;
        }
        case WM_MEDICAT_VENTOY_VERSIONS: {
            auto* payload = reinterpret_cast<std::vector<std::wstring>*>(lp);
            if (payload) {
                self->SetVentoyVersions(std::move(*payload));
                delete payload;
            } else {
                self->ventoyVersionsLoading_ = false;
            }
            return 0;
        }
        case WM_DESTROY:
            if (self->fileLogWindow_ && IsWindow(self->fileLogWindow_)) {
                DestroyWindow(self->fileLogWindow_);
            }
            if (self->logoBitmap_) {
                DeleteObject(self->logoBitmap_);
                self->logoBitmap_ = nullptr;
            }
            theme::Shutdown();
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void Gui::OnCreate(HWND hwnd) {
    hwnd_ = hwnd;
    const HFONT uiFont = theme::MakeUiFont();
    const HFONT titleFont = theme::MakeTitleFont();
    const HFONT subtitleFont = theme::MakeSubtitleFont();

    logoBitmap_ = theme::LoadLogoBitmap(instance_, kLogoMaxSize);
    if (!logoBitmap_) {
        logoStatic_ = CreateWindowW(
            L"STATIC", nullptr, WS_CHILD | WS_VISIBLE | SS_ICON | SS_LEFT,
            kHeaderLogoX, kHeaderLogoY, kLogoMaxSize, kLogoMaxSize, hwnd, nullptr, instance_, nullptr);
        const HICON fallbackIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
        if (fallbackIcon) {
            SendMessageW(logoStatic_, STM_SETICON, 0, reinterpret_cast<LPARAM>(fallbackIcon));
        }
    }

    languageCombo_ = CreateWindowW(
        WC_COMBOBOXW, nullptr,
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL,
        kLanguageComboX, kLanguageComboY, kLanguageComboWidth, 120, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLanguageComboId)), instance_, nullptr);
    SendMessageW(languageCombo_, CB_SETDROPPEDWIDTH, kLanguageComboWidth + 24, 0);
    SendMessageW(languageCombo_, CB_SETMINVISIBLE, 5, 0);
    for (const LanguageOption& option : kLanguageOptions) {
        SendMessageW(languageCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.nativeName));
    }
    SendMessageW(languageCombo_, CB_SETCURSEL, LanguageIndexForCode(i18n::DetectLanguage()), 0);

    titleLabel_ = CreateWindowW(
        L"STATIC", i18n::Tr(L"ui.title_label").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_LEFTNOWORDWRAP,
        kHeaderTitleX, kHeaderTitleY, kHeaderTitleWidth, 28, hwnd, nullptr, instance_, nullptr);

    wchar_t versionText[32]{};
    swprintf_s(versionText, L"v%hs", INSTALLER_VERSION);
    versionLabel_ = CreateWindowW(
        L"STATIC", versionText,
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        kMargin, 0, kContentWidth, kVersionLabelHeight, hwnd, nullptr, instance_, nullptr);

    driveLabel_ = CreateWindowW(
        L"STATIC", i18n::Tr(L"ui.drive_label").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_LEFTNOWORDWRAP,
        kMargin, kContentTop, kContentWidth, 24, hwnd, nullptr, instance_, nullptr);
    driveCombo_ = CreateWindowW(
        WC_COMBOBOXW, nullptr,
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        kMargin, kDriveComboY, kContentWidth, 300, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDriveComboId)), instance_, nullptr);

    showAllDrivesCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.show_all_drives").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, kShowAllDrivesY, kContentWidth, kCheckboxHeight + 8, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kShowAllDrivesCheckId)), instance_, nullptr);

    formatCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.format_checkbox").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, kFormatY, kContentWidth, kCheckboxHeight + 8, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kFormatCheckId)), instance_, nullptr);
    SendMessageW(formatCheck_, BM_SETCHECK, BST_CHECKED, 0);

    skipVentoyCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.skip_ventoy_checkbox").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, kSkipVentoyY, kContentWidth, kCheckboxHeight + 8, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSkipVentoyCheckId)), instance_, nullptr);

    advancedCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.advanced_options").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, kAdvancedY, 260, kCheckboxHeight + 8, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAdvancedCheckId)), instance_, nullptr);

    pinVentoyCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.pin_ventoy_version").c_str(),
        WS_CHILD | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin + 20, kPinVentoyY, 260, kCheckboxHeight + 8, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPinVentoyCheckId)), instance_, nullptr);

    ventoyVersionCombo_ = CreateWindowW(
        WC_COMBOBOXW, nullptr,
        CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL,
        270, kVentoyComboY, 270, 300, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVentoyVersionEditId)), instance_, nullptr);

    ventoySecureBootCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.ventoy_secure_boot").c_str(),
        WS_CHILD | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin + 20, kVentoySecureBootY, kContentWidth - 20, kCheckboxHeight + 8, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVentoySecureBootCheckId)), instance_, nullptr);
    SendMessageW(ventoySecureBootCheck_, BM_SETCHECK, BST_CHECKED, 0);

    ventoyGptCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.ventoy_gpt_partition").c_str(),
        WS_CHILD | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin + 20, kGptY, kContentWidth - 20, kCheckboxHeight + 8, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVentoyGptCheckId)), instance_, nullptr);

    installBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.install_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        kMargin, kInstallYCollapsed, kInstallBtnWidth, kInstallBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kInstallBtnId)), instance_, nullptr);

    verifyFilesBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.check_files_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        kVerifyBtnX, kInstallYCollapsed, kVerifyBtnWidth, kInstallBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVerifyFilesBtnId)), instance_, nullptr);

    progressBar_ = CreateWindowW(
        PROGRESS_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        kMargin, kProgressYCollapsed, 360, kProgressHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kProgressId)), instance_, nullptr);
    SendMessageW(progressBar_, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SetPropW(progressBar_, L"MedicatGui", this);
    const auto originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        progressBar_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ProgressBarProc)));
    SetWindowLongPtrW(progressBar_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(originalProc));

    openLogBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.open_file_log_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        390, kOpenLogYCollapsed, 150, kOpenLogBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOpenLogBtnId)), instance_, nullptr);

    currentFileLabel_ = CreateWindowW(
        L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS,
        kMargin, kCurrentFileYCollapsed, kContentWidth, kCurrentFileHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCurrentFileLabelId)), instance_, nullptr);

    for (HWND child :
         {languageCombo_, titleLabel_, versionLabel_, driveLabel_, driveCombo_, showAllDrivesCheck_,
          formatCheck_, skipVentoyCheck_, advancedCheck_, pinVentoyCheck_, ventoySecureBootCheck_, ventoyGptCheck_,
          ventoyVersionCombo_, installBtn_, verifyFilesBtn_, openLogBtn_, progressBar_, currentFileLabel_}) {
        if (child && IsWindow(child)) {
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        }
    }
    SendMessageW(titleLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont), TRUE);
    SendMessageW(versionLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(subtitleFont), TRUE);

    theme::EnableDarkMode(hwnd);
    theme::EnableDarkModeRecursive(hwnd);

    SubclassGlowButton(installBtn_, true);
    SubclassGlowButton(verifyFilesBtn_, false);
    SubclassGlowButton(openLogBtn_, false);

    for (const HWND checkbox : {formatCheck_, skipVentoyCheck_, showAllDrivesCheck_, advancedCheck_, pinVentoyCheck_,
                                ventoySecureBootCheck_, ventoyGptCheck_}) {
        SubclassFlatCheckbox(checkbox);
        InvalidateRect(checkbox, nullptr, TRUE);
    }

    UpdateAdvancedControls();
    LayoutHeader();
    LayoutVersionLabel();
    RefreshDrives();
    RefreshTranslatedUi();
}

void Gui::OnCommand(WPARAM wp) {
    const int id = LOWORD(wp);
    if (id == kLanguageComboId && HIWORD(wp) == CBN_SELCHANGE) {
        const int selected = static_cast<int>(SendMessageW(languageCombo_, CB_GETCURSEL, 0, 0));
        ApplyLanguageSelection(LanguageCodeForIndex(selected));
        return;
    }
    if (id == kShowAllDrivesCheckId) {
        RefreshDrives();
        return;
    }
    if (id == kAdvancedCheckId || id == kPinVentoyCheckId) {
        UpdateAdvancedControls();
        return;
    }
    if (id == kInstallBtnId && onInstall_) {
        onInstall_();
        return;
    }
    if (id == kVerifyFilesBtnId && onVerify_) {
        onVerify_();
        return;
    }
    if (id == kOpenLogBtnId) {
        OpenFileLogWindow();
    }
}

void Gui::ApplyLanguageSelection(const std::wstring& languageCode) {
    const int selected = LanguageIndexForCode(languageCode);
    i18n::Load(languageCode);
    if (languageCombo_ && IsWindow(languageCombo_)) {
        SendMessageW(languageCombo_, CB_SETCURSEL, selected, 0);
    }
    RefreshTranslatedUi();
}

void Gui::RefreshTranslatedUi() {
    const auto refreshControl = [](HWND child) {
        if (child && IsWindow(child)) {
            RedrawWindow(child, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
        }
    };

    if (titleLabel_ && IsWindow(titleLabel_)) {
        SetWindowTextW(titleLabel_, i18n::Tr(L"ui.title_label").c_str());
    }
    if (driveLabel_ && IsWindow(driveLabel_)) {
        SetWindowTextW(driveLabel_, i18n::Tr(L"ui.drive_label").c_str());
    }
    if (driveCombo_ && IsWindow(driveCombo_)) {
        RefreshDrives();
    }
    if (showAllDrivesCheck_ && IsWindow(showAllDrivesCheck_)) {
        SetWindowTextW(showAllDrivesCheck_, i18n::Tr(L"ui.show_all_drives").c_str());
    }
    if (formatCheck_ && IsWindow(formatCheck_)) {
        SetWindowTextW(formatCheck_, i18n::Tr(L"ui.format_checkbox").c_str());
    }
    if (skipVentoyCheck_ && IsWindow(skipVentoyCheck_)) {
        SetWindowTextW(skipVentoyCheck_, i18n::Tr(L"ui.skip_ventoy_checkbox").c_str());
    }
    if (advancedCheck_ && IsWindow(advancedCheck_)) {
        SetWindowTextW(advancedCheck_, i18n::Tr(L"ui.advanced_options").c_str());
    }
    if (pinVentoyCheck_ && IsWindow(pinVentoyCheck_)) {
        SetWindowTextW(pinVentoyCheck_, i18n::Tr(L"ui.pin_ventoy_version").c_str());
    }
    if (ventoySecureBootCheck_ && IsWindow(ventoySecureBootCheck_)) {
        SetWindowTextW(ventoySecureBootCheck_, i18n::Tr(L"ui.ventoy_secure_boot").c_str());
    }
    if (ventoyGptCheck_ && IsWindow(ventoyGptCheck_)) {
        SetWindowTextW(ventoyGptCheck_, i18n::Tr(L"ui.ventoy_gpt_partition").c_str());
    }
    if (installBtn_ && IsWindow(installBtn_)) {
        SetWindowTextW(installBtn_, i18n::Tr(L"ui.install_button").c_str());
    }
    if (verifyFilesBtn_ && IsWindow(verifyFilesBtn_)) {
        SetWindowTextW(verifyFilesBtn_, i18n::Tr(L"ui.check_files_button").c_str());
    }
    if (openLogBtn_ && IsWindow(openLogBtn_)) {
        SetWindowTextW(openLogBtn_, i18n::Tr(L"ui.open_file_log_button").c_str());
    }
    if (fileLogWindow_ && IsWindow(fileLogWindow_)) {
        SetWindowTextW(fileLogWindow_, i18n::Tr(L"ui.file_log_title").c_str());
    }
    if (versionLabel_ && IsWindow(versionLabel_)) {
        wchar_t versionText[32]{};
        swprintf_s(versionText, L"v%hs", INSTALLER_VERSION);
        SetWindowTextW(versionLabel_, versionText);
    }

    for (HWND child : {titleLabel_, versionLabel_, driveLabel_, driveCombo_, showAllDrivesCheck_, formatCheck_,
                       skipVentoyCheck_, advancedCheck_, pinVentoyCheck_, ventoySecureBootCheck_, ventoyGptCheck_,
                       installBtn_, verifyFilesBtn_, openLogBtn_, progressBar_, currentFileLabel_, languageCombo_}) {
        refreshControl(child);
    }
    if (hwnd_ && IsWindow(hwnd_)) {
        RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    }
}

void Gui::RefreshDrives() {
    std::wstring previous;
    const int previousIdx = static_cast<int>(SendMessageW(driveCombo_, CB_GETCURSEL, 0, 0));
    if (previousIdx >= 0) {
        const auto* letter =
            reinterpret_cast<std::wstring*>(SendMessageW(driveCombo_, CB_GETITEMDATA, previousIdx, 0));
        if (letter) {
            previous = *letter;
        }
    }

    const int count = static_cast<int>(SendMessageW(driveCombo_, CB_GETCOUNT, 0, 0));
    for (int i = 0; i < count; ++i) {
        const auto* letter = reinterpret_cast<std::wstring*>(SendMessageW(driveCombo_, CB_GETITEMDATA, i, 0));
        delete letter;
    }

    SendMessageW(driveCombo_, CB_RESETCONTENT, 0, 0);
    const auto drives = ListTargetDrives(ShowAllDrivesChecked());
    int restoreIdx = -1;
    for (size_t i = 0; i < drives.size(); ++i) {
        const auto& d = drives[i];
        SendMessageW(driveCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(d.display.c_str()));
        const int idx = static_cast<int>(SendMessageW(driveCombo_, CB_GETCOUNT, 0, 0)) - 1;
        SendMessageW(driveCombo_, CB_SETITEMDATA, idx, reinterpret_cast<LPARAM>(new std::wstring(d.letter)));
        if (!previous.empty() && d.letter == previous) {
            restoreIdx = idx;
        }
    }

    const int def = restoreIdx >= 0 ? restoreIdx : DefaultDriveIndex(drives);
    if (def >= 0) {
        SendMessageW(driveCombo_, CB_SETCURSEL, def, 0);
    }
}

}  // namespace medicat
