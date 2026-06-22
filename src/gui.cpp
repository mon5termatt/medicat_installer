#include "gui.h"

#include "archive.h"
#include "drives.h"
#include "download.h"
#include "downloads.h"
#include "i18n.h"
#include "offline.h"
#include "resource.h"
#include "theme.h"
#include "util.h"
#include "ventoy.h"

#include <commctrl.h>
#include <dbt.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <thread>

namespace medicat {

namespace {

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
constexpr int kVersionLabelGap = 4;
constexpr int kVersionLabelY = kLanguageComboY - kVersionLabelHeight - kVersionLabelGap;
constexpr int kVersionLabelWidth = kContentWidth - (kLanguageComboX - kMargin);
constexpr int kContentTop = kHeaderHeight + 10;

constexpr int kCheckboxHeight = 24;
constexpr int kSectionGap = 12;
constexpr int kInstallBtnHeight = 44;
constexpr int kInstallBtnWidth = 252;
constexpr int kActionBtnGap = 10;
constexpr int kVerifyBtnWidth = kContentWidth - kInstallBtnWidth - kActionBtnGap;
constexpr int kOpenLogBtnWidth = 150;
constexpr int kProgressBarWidth = kContentWidth - kOpenLogBtnWidth - kActionBtnGap;
constexpr int kProgressHeight = 28;
constexpr int kOpenLogBtnHeight = 32;
constexpr int kManualInstallBtnHeight = 32;
constexpr int kManualInstallGap = 10;
constexpr int kCreditsBtnHeight = 28;
constexpr int kCreditsBtnGap = 8;
constexpr int kFooterBtnGap = 8;
constexpr int kDiscordFooterBtnWidth = kCreditsBtnHeight;
constexpr int kDiscordFooterIconSize = kCreditsBtnHeight;
constexpr int kActionRowGap = 12;
constexpr int kStatusBarHeight = 20;
constexpr int kBetaNoticeMinHeight = 20;
constexpr int kBetaNoticeGap = 4;
constexpr int kCheckboxRowHeight = kCheckboxHeight + 8;
constexpr int kProgressRowHeight = std::max(kProgressHeight, kOpenLogBtnHeight);
constexpr int kBottomChrome = 28;
constexpr int kArchiveLabelHeight = 36;
constexpr int kDownloadBtnHeight = 28;
constexpr int kDownloadBtnGap = 8;
constexpr int kArchivePanelHeight = kArchiveLabelHeight + kDownloadBtnHeight + kDownloadBtnGap + kDownloadBtnHeight + 10;

struct MainContentLayout {
    int contentTop = 0;
    int driveComboY = 0;
    int showAllDrivesY = 0;
    int formatY = 0;
    int ventoyActionY = 0;
    int advancedY = 0;
    int pinVentoyY = 0;
    int ventoySecureBootY = 0;
    int gptY = 0;
    int installY = 0;
    int progressY = 0;
    int openLogY = 0;
    int statusBarY = 0;
    int betaNoticeY = 0;
    int betaNoticeHeight = kBetaNoticeMinHeight;
    int manualInstallY = 0;
    int creditsBtnY = 0;
    int requiredClientHeight = 0;
};

MainContentLayout ComputeMainContentLayout(const bool expanded, const bool archiveMissing,
                                           const int betaNoticeHeight = kBetaNoticeMinHeight) {
    MainContentLayout layout{};
    layout.contentTop = archiveMissing ? (kContentTop + kArchivePanelHeight) : kContentTop;
    layout.driveComboY = layout.contentTop + 22;
    layout.showAllDrivesY = layout.driveComboY + 32;
    layout.formatY = layout.showAllDrivesY + 28;
    layout.ventoyActionY = layout.formatY + 30;
    layout.advancedY = layout.ventoyActionY + 30;
    layout.pinVentoyY = layout.advancedY + 28;
    layout.ventoySecureBootY = layout.pinVentoyY + 28;
    layout.gptY = layout.ventoySecureBootY + 30;

    const int optionsBottom =
        expanded ? (layout.gptY + kCheckboxRowHeight) : (layout.advancedY + kCheckboxRowHeight);
    layout.installY = optionsBottom + kSectionGap;
    layout.progressY = layout.installY + kInstallBtnHeight + kActionRowGap;
    layout.openLogY = layout.progressY + (kProgressHeight - kOpenLogBtnHeight) / 2;
    layout.statusBarY = layout.progressY + kProgressRowHeight + 8;
    layout.betaNoticeY = layout.statusBarY + kStatusBarHeight + kBetaNoticeGap;
    layout.betaNoticeHeight = betaNoticeHeight;
    layout.manualInstallY = layout.betaNoticeY + betaNoticeHeight + kManualInstallGap;
    layout.creditsBtnY = layout.manualInstallY + kManualInstallBtnHeight + kCreditsBtnGap;
    layout.requiredClientHeight = layout.creditsBtnY + kCreditsBtnHeight + kBottomChrome;
    return layout;
}

int OuterWindowHeightForClient(HWND hwnd, const int clientWidth, const int clientHeight) {
    RECT frame{0, 0, clientWidth, clientHeight};
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    AdjustWindowRectEx(&frame, style, FALSE, exStyle);
    return frame.bottom - frame.top;
}

int EstimateInitialOuterHeight(const bool archiveMissing) {
    const auto layout = ComputeMainContentLayout(false, archiveMissing);
    RECT frame{0, 0, kContentWidth + 2 * kMargin, layout.requiredClientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);
    return frame.bottom - frame.top;
}

int EstimateInitialOuterWidth() {
    RECT frame{0, 0, kContentWidth + 2 * kMargin, 100};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);
    return frame.right - frame.left;
}

int ContentLeft(const int clientWidth) {
    if (clientWidth <= kContentWidth + 2 * kMargin) {
        return kMargin;
    }
    return (clientWidth - kContentWidth) / 2;
}

struct FooterButtonLayout {
    int sideBtnWidth = 0;
    int discordBtnWidth = kDiscordFooterBtnWidth;
    int creditsX = 0;
    int discordX = 0;
    int feedbackX = 0;
};

FooterButtonLayout ComputeFooterButtonLayout(const int contentLeft) {
    FooterButtonLayout layout{};
    layout.sideBtnWidth =
        (kContentWidth - 2 * kFooterBtnGap - layout.discordBtnWidth) / 2;
    layout.creditsX = contentLeft;
    layout.discordX = contentLeft + layout.sideBtnWidth + kFooterBtnGap;
    layout.feedbackX = layout.discordX + layout.discordBtnWidth + kFooterBtnGap;
    return layout;
}

int OuterWindowWidthForClient(HWND hwnd, const int clientWidth) {
    RECT frame{0, 0, clientWidth, 100};
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    AdjustWindowRectEx(&frame, style, FALSE, exStyle);
    return frame.right - frame.left;
}

constexpr int kDriveComboId = 1001;
constexpr int kLanguageComboId = 1018;
constexpr int kShowAllDrivesCheckId = 1016;
constexpr int kFormatCheckId = 1002;
constexpr int kVentoyActionCheckId = 1003;
constexpr int kInstallBtnId = 1004;
constexpr int kProgressId = 1005;
constexpr int kStatusId = 1006;
constexpr int kAdvancedCheckId = 1007;
constexpr int kPinVentoyCheckId = 1008;
constexpr int kVentoyVersionEditId = 1009;
constexpr int kOpenLogBtnId = 1010;
constexpr int kFileLogListId = 1011;
constexpr int kStatusBarId = 1012;
constexpr int kVentoySecureBootCheckId = 1013;
constexpr int kVentoyGptCheckId = 1014;
constexpr int kVerifyFilesBtnId = 1015;
constexpr int kDownloadMirror1BtnId = 1020;
constexpr int kDownloadMirror2BtnId = 1021;
constexpr int kAltDownloadComboId = 1022;
constexpr int kAltDownloadOpenBtnId = 1023;
constexpr int kManualInstallBtnId = 1024;
constexpr int kCreditsBtnId = 1025;
constexpr int kFeedbackBtnId = 1026;
constexpr int kDiscordFooterBtnId = 1027;
constexpr int kCreditsIntroId = 1130;
constexpr int kCreditsSevenZipBtnId = 1131;
constexpr int kCreditsVentoyBtnId = 1132;
constexpr int kCreditsCloseBtnId = 1133;
constexpr int kReExtractMessageId = 1101;
constexpr int kReExtractListId = 1102;
constexpr int kReExtractBtnId = 1103;
constexpr int kReExtractCloseBtnId = 1104;
constexpr int kMsgDialogMessageId = 1140;
constexpr int kMsgDialogDiagLabelId = 1141;
constexpr int kMsgDialogDiagEditId = 1142;
constexpr int kMsgDialogCopyBtnId = 1143;
constexpr int kMsgDialogDiscordBtnId = 1147;
constexpr int kMsgDialogOkBtnId = 1144;
constexpr int kMsgDialogYesBtnId = 1145;
constexpr int kMsgDialogNoBtnId = 1146;
constexpr UINT_PTR kUiRefreshTimerId = 1;
constexpr UINT_PTR kArchiveCheckTimerId = 2;
constexpr UINT_PTR kDriveRefreshTimerId = 3;
constexpr UINT_PTR kUpdateCheckTimerId = 4;
constexpr UINT kUpdateCheckDelayMs = 500;
constexpr UINT kUpdateCheckIntervalMs = 5 * 60 * 1000;  // 5 minutes
constexpr UINT kUiRefreshIntervalMs = 250;
constexpr UINT kArchiveCheckIntervalMs = 3000;
constexpr UINT kDriveDebounceMs = 500;
constexpr int kMirrorBtnWidth = (kContentWidth - kDownloadBtnGap) / 2;
constexpr int kAltComboWidth = 360;
constexpr int kAltOpenBtnWidth = kContentWidth - kAltComboWidth - kDownloadBtnGap;
constexpr int kAltOpenBtnX = kMargin + kAltComboWidth + kDownloadBtnGap;
constexpr wchar_t kFileLogWindowClass[] = L"MedicatFileLogWindow";
constexpr wchar_t kCreditsWindowClass[] = L"MedicatCreditsWindow";
constexpr wchar_t kReExtractWindowClass[] = L"MedicatReExtractWindow";
constexpr wchar_t kScrollableTextPaneClass[] = L"MedicatScrollableTextPane";
constexpr wchar_t kMessageDialogWindowClass[] = L"MedicatMessageDialogWindow";
constexpr int kMsgDialogClientWidth = 480;
constexpr int kMsgDialogMinClientHeight = 220;
constexpr int kMsgDialogMaxMessageHeight = 240;
constexpr int kMsgDialogAccentHeight = 3;
constexpr int kMsgDialogMargin = 24;
constexpr int kMsgDialogTopPad = 20;
constexpr int kMsgDialogBtnHeight = 34;
constexpr int kMsgDialogBtnGap = 10;
constexpr int kMsgDialogSectionGap = 16;
constexpr int kMsgDialogDiagEditHeight = 32;

constexpr wchar_t kHelpGateWindowClass[] = L"MedicatHelpGateWindow";
constexpr int kHelpGateBodyId = 1150;
constexpr int kHelpGateDiscordBtnId = 1151;
constexpr int kHelpGateAckCheckboxId = 1152;
constexpr int kHelpGateContinueBtnId = 1153;
constexpr int kHelpGateClientWidth = 500;
constexpr int kHelpGateMinClientHeight = 320;
constexpr int kHelpGateMaxMessageHeight = 200;
constexpr int kHelpGateDiscordBtnHeight = 56;
constexpr int kHelpGateAckHeight = 48;
constexpr COLORREF kHelpGateAccentColor = RGB(230, 176, 60);
constexpr int kReExtractClientWidth = 480;
constexpr int kReExtractMinClientHeight = 300;
constexpr int kReExtractMaxMessageHeight = 160;
constexpr int kReExtractMinListHeight = 100;
constexpr int kReExtractMaxListHeight = 280;
constexpr int kReExtractRowHeight = 18;
constexpr int kReExtractDialogMargin = 24;
constexpr int kReExtractDialogTopPad = 20;
constexpr int kReExtractDialogSectionGap = 16;
constexpr int kReExtractDialogBtnHeight = 34;
constexpr int kReExtractDialogBtnGap = 10;
constexpr int kCreditsClientWidth = 400;
constexpr int kCreditsClientHeight = 268;
constexpr int kCreditsDialogMargin = 24;
constexpr int kCreditsDialogTopPad = 20;
constexpr int kCreditsDialogBtnHeight = 34;
constexpr int kCreditsDialogBtnGap = 10;
constexpr int kCreditsDialogSectionGap = 20;

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
    {L"cat", L"Cat"},
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

std::wstring FormatDownloadSizeText(const uint64_t downloaded, const uint64_t total) {
    if (total > 0) {
        return FormatProgressBytes(downloaded) + L" / " + FormatProgressBytes(total);
    }
    return FormatProgressBytes(downloaded);
}

int DownloadPercent(const uint64_t downloaded, const uint64_t total) {
    if (total == 0) {
        return 0;
    }
    return static_cast<int>((downloaded * 100) / total);
}

int CreditsOuterWidth() {
    RECT frame{0, 0, kCreditsClientWidth, kCreditsClientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.right - frame.left;
}

int CreditsOuterHeight() {
    RECT frame{0, 0, kCreditsClientWidth, kCreditsClientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.bottom - frame.top;
}

int MessageDialogOuterWidth() {
    RECT frame{0, 0, kMsgDialogClientWidth, kMsgDialogMinClientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.right - frame.left;
}

int MessageDialogOuterHeightForMessage(const int messageHeight, const MessageDialogFooter footer) {
    const int cappedMessage = std::min(messageHeight, kMsgDialogMaxMessageHeight);
    const int maxClientHeight = (GetSystemMetrics(SM_CYSCREEN) * 85) / 100;
    int clientHeight = kMsgDialogAccentHeight + kMsgDialogTopPad + cappedMessage + kMsgDialogSectionGap;
    if (footer == MessageDialogFooter::OkWithDiag) {
        clientHeight += 18 + 6 + kMsgDialogDiagEditHeight + kMsgDialogSectionGap;
    }
    clientHeight += kMsgDialogBtnHeight + 16;
    clientHeight = std::max(clientHeight, kMsgDialogMinClientHeight);
    clientHeight = std::min(clientHeight, maxClientHeight);

    RECT frame{0, 0, kMsgDialogClientWidth, clientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.bottom - frame.top;
}

int HelpGateOuterWidth() {
    RECT frame{0, 0, kHelpGateClientWidth, kHelpGateMinClientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.right - frame.left;
}

int HelpGateOuterHeightForMessage(const int messageHeight) {
    const int cappedMessage = std::min(messageHeight, kHelpGateMaxMessageHeight);
    const int maxClientHeight = (GetSystemMetrics(SM_CYSCREEN) * 85) / 100;
    int clientHeight = kMsgDialogAccentHeight + kMsgDialogTopPad + cappedMessage + kMsgDialogSectionGap +
                       kHelpGateDiscordBtnHeight + kMsgDialogSectionGap + kHelpGateAckHeight + kMsgDialogSectionGap +
                       kMsgDialogBtnHeight + 16;
    clientHeight = std::max(clientHeight, kHelpGateMinClientHeight);
    clientHeight = std::min(clientHeight, maxClientHeight);

    RECT frame{0, 0, kHelpGateClientWidth, clientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.bottom - frame.top;
}

int ReExtractOuterWidth() {
    RECT frame{0, 0, kReExtractClientWidth, kReExtractMinClientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.right - frame.left;
}

int ReExtractOuterHeightForContent(const int messageHeight, const int listHeight) {
    const int maxClientHeight = (GetSystemMetrics(SM_CYSCREEN) * 85) / 100;
    const int cappedMessage = std::min(messageHeight, kReExtractMaxMessageHeight);
    int clientHeight = kMsgDialogAccentHeight + kReExtractDialogTopPad + cappedMessage + kReExtractDialogSectionGap +
                       listHeight + kReExtractDialogSectionGap + kReExtractDialogBtnHeight + 16;
    clientHeight = std::max(clientHeight, kReExtractMinClientHeight);
    clientHeight = std::min(clientHeight, maxClientHeight);

    RECT frame{0, 0, kReExtractClientWidth, clientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    return frame.bottom - frame.top;
}

bool CopyTextToClipboard(HWND owner, const std::wstring& text) {
    if (text.empty()) {
        return false;
    }
    if (!OpenClipboard(owner)) {
        return false;
    }
    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) {
        CloseClipboard();
        return false;
    }
    void* locked = GlobalLock(memory);
    if (!locked) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    memcpy(locked, text.c_str(), bytes);
    GlobalUnlock(memory);
    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    CloseClipboard();
    return true;
}

std::vector<std::wstring> CollectComboDriveLetters(const HWND combo) {
    std::vector<std::wstring> letters;
    if (!combo || !IsWindow(combo)) {
        return letters;
    }

    const int count = static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0));
    letters.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const auto* letter = reinterpret_cast<std::wstring*>(SendMessageW(combo, CB_GETITEMDATA, i, 0));
        if (letter && !letter->empty()) {
            letters.push_back(*letter);
        }
    }
    return letters;
}

int MeasureWrappedStaticHeight(HWND hwnd, const std::wstring& text, const int width,
                               const int minHeight = 32) {
    if (!hwnd || text.empty() || width <= 0) {
        return minHeight;
    }

    const HDC hdc = GetDC(hwnd);
    if (!hdc) {
        return minHeight;
    }

    const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    HFONT oldFont = nullptr;
    if (font) {
        oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, font));
    }

    RECT rc{0, 0, width, 0};
    DrawTextW(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);

    if (oldFont) {
        SelectObject(hdc, oldFont);
    }
    ReleaseDC(hwnd, hdc);
    return std::max(minHeight, static_cast<int>(rc.bottom - rc.top) + 4);
}

struct WrappedStaticState {
    WNDPROC original = nullptr;
    bool muted = false;
};

LRESULT CALLBACK WrappedStaticProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
    auto* state = reinterpret_cast<WrappedStaticState*>(GetPropW(hwnd, L"MedicatWrappedStatic"));
    if (!state || !state->original) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    if (msg == WM_PAINT) {
        PAINTSTRUCT ps{};
        const HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc{};
        GetClientRect(hwnd, &rc);

        wchar_t text[1024]{};
        GetWindowTextW(hwnd, text, static_cast<int>(std::size(text)));

        const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
        HFONT oldFont = nullptr;
        if (font) {
            oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, font));
        }

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, state->muted ? theme::Colors().muted : theme::Colors().text);
        DrawTextW(hdc, text, -1, &rc, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

        if (oldFont) {
            SelectObject(hdc, oldFont);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        return 1;
    }
    if (msg == WM_NCDESTROY) {
        RemovePropW(hwnd, L"MedicatWrappedStatic");
        delete state;
    }
    return CallWindowProcW(state->original, hwnd, msg, wp, lp);
}

void SubclassWrappedStatic(const HWND hwnd, const bool muted) {
    auto* state = new WrappedStaticState{};
    state->original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WrappedStaticProc)));
    state->muted = muted;
    SetPropW(hwnd, L"MedicatWrappedStatic", state);
}

struct ScrollableTextPaneState {
    std::wstring text;
    int scrollY = 0;
    int contentHeight = 0;
    int viewportHeight = 0;
    int contentWidth = 0;
    int lineHeight = 20;
};

ScrollableTextPaneState* GetScrollableTextPaneState(const HWND hwnd) {
    return reinterpret_cast<ScrollableTextPaneState*>(GetPropW(hwnd, L"MedicatScrollableTextPane"));
}

int MeasureScrollableTextPaneContentHeight(const HDC hdc, const HFONT font, const std::wstring& text,
                                           const int width) {
    if (text.empty() || width <= 0) {
        return 32;
    }

    HFONT oldFont = nullptr;
    if (font) {
        oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, font));
    }

    RECT rc{0, 0, width, 0};
    DrawTextW(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);

    if (oldFont) {
        SelectObject(hdc, oldFont);
    }
    return std::max(32, static_cast<int>(rc.bottom - rc.top) + 4);
}

void UpdateScrollableTextPaneMetrics(const HWND hwnd, ScrollableTextPaneState* state) {
    if (!hwnd || !IsWindow(hwnd) || !state) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd, &client);
    state->viewportHeight = std::max(0, static_cast<int>(client.bottom - client.top));
    state->contentWidth = std::max(0, static_cast<int>(client.right - client.left - 8));

    const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    const HDC hdc = GetDC(hwnd);
    if (hdc) {
        if (font) {
            const HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, font));
            TEXTMETRICW metrics{};
            if (GetTextMetricsW(hdc, &metrics)) {
                state->lineHeight = std::max(16, static_cast<int>(metrics.tmHeight + metrics.tmExternalLeading));
            }
            state->contentHeight =
                MeasureScrollableTextPaneContentHeight(hdc, font, state->text, state->contentWidth);
            if (oldFont) {
                SelectObject(hdc, oldFont);
            }
        } else {
            state->contentHeight =
                MeasureScrollableTextPaneContentHeight(hdc, nullptr, state->text, state->contentWidth);
        }
        ReleaseDC(hwnd, hdc);
    }

    const int maxScroll = std::max(0, state->contentHeight - state->viewportHeight);
    state->scrollY = std::clamp(state->scrollY, 0, maxScroll);

    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = state->contentHeight;
    si.nPage = static_cast<UINT>(std::max(1, state->viewportHeight));
    si.nPos = state->scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    ShowScrollBar(hwnd, SB_VERT, maxScroll > 0 ? TRUE : FALSE);
}

void ScrollableTextPaneScrollBy(const HWND hwnd, ScrollableTextPaneState* state, const int delta) {
    if (!hwnd || !state) {
        return;
    }
    const int maxScroll = std::max(0, state->contentHeight - state->viewportHeight);
    const int next = std::clamp(state->scrollY + delta, 0, maxScroll);
    if (next == state->scrollY) {
        return;
    }
    state->scrollY = next;

    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = state->scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void SetScrollableTextPaneText(const HWND hwnd, const std::wstring& text) {
    ScrollableTextPaneState* state = GetScrollableTextPaneState(hwnd);
    if (!state) {
        return;
    }
    state->text = text;
    state->scrollY = 0;
    UpdateScrollableTextPaneMetrics(hwnd, state);
    InvalidateRect(hwnd, nullptr, TRUE);
}

void LayoutScrollableTextPane(const HWND hwnd, const int x, const int y, const int width, const int height) {
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    UpdateScrollableTextPaneMetrics(hwnd, GetScrollableTextPaneState(hwnd));
}

HWND CreateScrollableTextPane(const HWND parent, const HINSTANCE instance, const int controlId, const HFONT font,
                              const std::wstring& text) {
    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE, kScrollableTextPaneClass, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0, 0, 100, 100, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)), instance, nullptr);
    if (!hwnd) {
        return nullptr;
    }

    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SetScrollableTextPaneText(hwnd, text);
    return hwnd;
}

LRESULT CALLBACK ScrollableTextPaneProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
    ScrollableTextPaneState* state = GetScrollableTextPaneState(hwnd);

    switch (msg) {
        case WM_CREATE: {
            state = new ScrollableTextPaneState{};
            SetPropW(hwnd, L"MedicatScrollableTextPane", state);
            return 0;
        }
        case WM_DESTROY: {
            ScrollableTextPaneState* paneState = GetScrollableTextPaneState(hwnd);
            RemovePropW(hwnd, L"MedicatScrollableTextPane");
            delete paneState;
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_SETFONT:
        case WM_SIZE: {
            const LRESULT result = DefWindowProcW(hwnd, msg, wp, lp);
            state = GetScrollableTextPaneState(hwnd);
            UpdateScrollableTextPaneMetrics(hwnd, state);
            return result;
        }
        case WM_VSCROLL: {
            state = GetScrollableTextPaneState(hwnd);
            if (!state) {
                return 0;
            }
            SCROLLINFO si{};
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd, SB_VERT, &si);
            const int line = std::max(16, state->lineHeight);
            const int page = std::max(line, state->viewportHeight - line);
            int next = si.nPos;
            switch (LOWORD(wp)) {
                case SB_LINEUP:
                    next -= line;
                    break;
                case SB_LINEDOWN:
                    next += line;
                    break;
                case SB_PAGEUP:
                    next -= page;
                    break;
                case SB_PAGEDOWN:
                    next += page;
                    break;
                case SB_TOP:
                    next = 0;
                    break;
                case SB_BOTTOM:
                    next = std::max(0, state->contentHeight - state->viewportHeight);
                    break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
                    next = HIWORD(wp);
                    break;
                default:
                    return 0;
            }
            const int maxScroll = std::max(0, state->contentHeight - state->viewportHeight);
            state->scrollY = std::clamp(next, 0, maxScroll);
            si.fMask = SIF_POS;
            si.nPos = state->scrollY;
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSEWHEEL: {
            state = GetScrollableTextPaneState(hwnd);
            if (!state) {
                return 0;
            }
            const int delta = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
            ScrollableTextPaneScrollBy(hwnd, state, -delta * std::max(16, state->lineHeight * 3));
            return 0;
        }
        case WM_KEYDOWN: {
            state = GetScrollableTextPaneState(hwnd);
            if (!state) {
                break;
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && wp == 0x43) {
                if (!state->text.empty()) {
                    CopyTextToClipboard(hwnd, state->text);
                }
                return 0;
            }
            switch (wp) {
                case VK_UP:
                    ScrollableTextPaneScrollBy(hwnd, state, -state->lineHeight);
                    return 0;
                case VK_DOWN:
                    ScrollableTextPaneScrollBy(hwnd, state, state->lineHeight);
                    return 0;
                case VK_PRIOR:
                    ScrollableTextPaneScrollBy(hwnd, state, -std::max(state->lineHeight, state->viewportHeight - state->lineHeight));
                    return 0;
                case VK_NEXT:
                    ScrollableTextPaneScrollBy(hwnd, state, std::max(state->lineHeight, state->viewportHeight - state->lineHeight));
                    return 0;
                case VK_HOME:
                    ScrollableTextPaneScrollBy(hwnd, state, -state->contentHeight);
                    return 0;
                case VK_END:
                    ScrollableTextPaneScrollBy(hwnd, state, state->contentHeight);
                    return 0;
                default:
                    break;
            }
            break;
        }
        case WM_GETDLGCODE:
            return DLGC_WANTARROWS | DLGC_WANTCHARS;
        case WM_PAINT: {
            state = GetScrollableTextPaneState(hwnd);
            if (!state) {
                return DefWindowProcW(hwnd, msg, wp, lp);
            }

            PAINTSTRUCT ps{};
            const HDC hdc = BeginPaint(hwnd, &ps);
            RECT client{};
            GetClientRect(hwnd, &client);
            FillRect(hdc, &client, theme::GetBrushes().control);

            if (!state->text.empty() && state->contentWidth > 0) {
                const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
                HFONT oldFont = nullptr;
                if (font) {
                    oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, font));
                }

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, theme::Colors().text);

                const HRGN clip = CreateRectRgnIndirect(&client);
                SelectClipRgn(hdc, clip);

                constexpr int kPad = 4;
                RECT drawRc{kPad, kPad - state->scrollY, client.right - kPad,
                            kPad - state->scrollY + state->contentHeight};
                DrawTextW(hdc, state->text.c_str(), -1, &drawRc, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

                SelectClipRgn(hdc, nullptr);
                DeleteObject(clip);
                if (oldFont) {
                    SelectObject(hdc, oldFont);
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool RegisterScrollableTextPaneClass(const HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ScrollableTextPaneProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = theme::GetBrushes().control;
    wc.lpszClassName = kScrollableTextPaneClass;
    return RegisterClassExW(&wc) != 0;
}

struct GlowButtonState {
    WNDPROC original = nullptr;
    bool hovered = false;
    bool pressed = false;
    bool primary = false;
    HICON icon = nullptr;
    bool iconFullBleed = false;
};

void PaintIconButtonBackground(const HDC hdc, const RECT& rc) {
    FillRect(hdc, &rc, theme::GetBrushes().window);
}

void PaintGlowButtonClient(const HWND hwnd, const HDC hdc, const GlowButtonState* state) {
    RECT rc{};
    GetClientRect(hwnd, &rc);

    theme::ButtonState btnState = theme::ButtonState::Normal;
    if (!IsWindowEnabled(hwnd)) {
        btnState = theme::ButtonState::Disabled;
    } else if (state->pressed) {
        btnState = theme::ButtonState::Pressed;
    } else if (state->hovered) {
        btnState = theme::ButtonState::Hovered;
    }

    const auto style = state->primary ? theme::ButtonStyle::Primary : theme::ButtonStyle::Secondary;
    if (state->icon) {
        if (state->iconFullBleed) {
            PaintIconButtonBackground(hdc, rc);
        }
        theme::PaintFlatIconButton(hdc, rc, state->icon, style, btnState, state->iconFullBleed);
        return;
    }

    wchar_t text[256]{};
    GetWindowTextW(hwnd, text, static_cast<int>(std::size(text)));
    const HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    theme::PaintFlatButton(hdc, rc, text, font, style, btnState);
}

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
                if (!state->iconFullBleed) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_MOUSELEAVE:
            state->hovered = false;
            state->pressed = false;
            if (!state->iconFullBleed) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONDOWN:
            if (IsWindowEnabled(hwnd)) {
                state->pressed = true;
                SetCapture(hwnd);
                if (!state->iconFullBleed) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_LBUTTONUP: {
            const bool wasPressed = state->pressed;
            state->pressed = false;
            if (GetCapture() == hwnd) {
                ReleaseCapture();
            }
            if (!state->iconFullBleed) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
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
                if (!state->iconFullBleed) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            return 0;
        case WM_KEYUP:
            if ((wp == VK_SPACE || wp == VK_RETURN) && state->pressed) {
                state->pressed = false;
                if (!state->iconFullBleed) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                if (IsWindowEnabled(hwnd)) {
                    const int id = GetDlgCtrlID(hwnd);
                    SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, BN_CLICKED),
                                 reinterpret_cast<LPARAM>(hwnd));
                }
                return 0;
            }
            return 0;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_CAPTURECHANGED:
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT && IsWindowEnabled(hwnd)) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
                return TRUE;
            }
            return FALSE;
        case WM_ENABLE:
            state->hovered = false;
            state->pressed = false;
            if (!state->iconFullBleed) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_UPDATEUISTATE:
            if (!state->iconFullBleed) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case BM_SETSTATE:
            return 0;
        case WM_PRINTCLIENT: {
            const HDC hdc = reinterpret_cast<HDC>(wp);
            PaintGlowButtonClient(hwnd, hdc, state);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            const HDC hdc = BeginPaint(hwnd, &ps);
            PaintGlowButtonClient(hwnd, hdc, state);
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

void SubclassGlowIconButton(HWND hwnd, HICON icon, const bool fullBleed = false) {
    SetWindowTheme(hwnd, L"", L"");
    SendMessageW(hwnd, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    auto* state = new GlowButtonState{};
    state->icon = icon;
    state->iconFullBleed = fullBleed;
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

    WNDPROC origState{ state->original };

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
            InvalidateRect(hwnd, nullptr, FALSE);
            return CallWindowProcW(state->original, hwnd, msg, wp, lp);
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
            state = nullptr;
            break;
        default:
            break;
    }

    return CallWindowProcW(origState, hwnd, msg, wp, lp);
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

    WNDCLASSEXW creditsWc{};
    creditsWc.cbSize = sizeof(creditsWc);
    creditsWc.lpfnWndProc = Gui::CreditsWndProc;
    creditsWc.hInstance = instance;
    creditsWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    creditsWc.hbrBackground = theme::GetBrushes().window;
    creditsWc.lpszClassName = kCreditsWindowClass;
    RegisterClassExW(&creditsWc);

    WNDCLASSEXW reExtractWc{};
    reExtractWc.cbSize = sizeof(reExtractWc);
    reExtractWc.lpfnWndProc = Gui::ReExtractWndProc;
    reExtractWc.hInstance = instance;
    reExtractWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    reExtractWc.hbrBackground = theme::GetBrushes().window;
    reExtractWc.lpszClassName = kReExtractWindowClass;
    RegisterClassExW(&reExtractWc);

    RegisterScrollableTextPaneClass(instance);

    WNDCLASSEXW messageDialogWc{};
    messageDialogWc.cbSize = sizeof(messageDialogWc);
    messageDialogWc.lpfnWndProc = Gui::MessageDialogWndProc;
    messageDialogWc.hInstance = instance;
    messageDialogWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    messageDialogWc.hbrBackground = theme::GetBrushes().window;
    messageDialogWc.lpszClassName = kMessageDialogWindowClass;
    RegisterClassExW(&messageDialogWc);

    WNDCLASSEXW helpGateWc{};
    helpGateWc.cbSize = sizeof(helpGateWc);
    helpGateWc.lpfnWndProc = Gui::HelpGateWndProc;
    helpGateWc.hInstance = instance;
    helpGateWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    helpGateWc.hbrBackground = theme::GetBrushes().window;
    helpGateWc.lpszClassName = kHelpGateWindowClass;
    RegisterClassExW(&helpGateWc);

    const std::wstring windowTitle = i18n::Tr(L"ui.form_title", InstallerVersionWide());
    hwnd_ = CreateWindowExW(
        0, cls, windowTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, EstimateInitialOuterWidth(), EstimateInitialOuterHeight(false),
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

void Gui::SetLogHandler(std::function<void(const std::wstring&)> handler) { onLog_ = std::move(handler); }

void Gui::SetUpdateCheckHandler(std::function<void()> handler) { onUpdateCheck_ = std::move(handler); }

void Gui::SetApplyInstallerUpdateHandler(std::function<void(const InstallerUpdateInfo&)> handler) {
    onApplyInstallerUpdate_ = std::move(handler);
}

void Gui::ScheduleUpdateCheck() {
    if (!hwnd_ || !IsWindow(hwnd_)) {
        return;
    }
    SetTimer(hwnd_, kUpdateCheckTimerId, kUpdateCheckDelayMs, nullptr);
}

void Gui::ShowUpdatePrompt(const InstallerUpdateInfo& info) {
    if (!hwnd_ || !IsWindow(hwnd_)) {
        return;
    }
    const std::wstring releaseTag = info.releaseTag.empty() ? info.version : info.releaseTag;
    if (!releaseTag.empty() && releaseTag == lastUpdatePromptReleaseTag_) {
        return;
    }
    if (!releaseTag.empty()) {
        lastUpdatePromptReleaseTag_ = releaseTag;
    }

#ifndef INSTALLER_RELEASE_TAG
#define INSTALLER_RELEASE_TAG "unknown"
#endif

    const std::wstring localVersion = InstallerVersionLabel();
    const std::wstring localTag = [&]() {
        wchar_t buffer[64]{};
        swprintf_s(buffer, L"%hs", INSTALLER_RELEASE_TAG);
        return std::wstring(buffer);
    }();
    const std::wstring remoteVersion =
        info.version.empty() ? info.releaseTag : (info.version.rfind(L'v', 0) == 0 ? info.version : L"v" + info.version);

    const std::wstring message =
        i18n::Tr(L"update.available_message", localVersion, localTag, remoteVersion, info.releaseTag);
    const std::wstring title = i18n::Tr(L"update.available_title");
    if (ShowConfirmDialog(message, title, MessageDialogKind::Info)) {
        if (info.downloadUrl.empty()) {
            ShowMessageDialog(i18n::Tr(L"update.download_unavailable"), title, MessageDialogKind::Warning);
            return;
        }
        if (onApplyInstallerUpdate_) {
            onApplyInstallerUpdate_(info);
        }
    }
}

void Gui::LogVentoyDetection(const std::wstring& drive, const VentoyDetectionResult& detection) {
    if (!onLog_ || drive.empty()) {
        return;
    }

    const bool changed =
        !hasLastVentoyLog_ || drive != lastVentoyLogDrive_ || detection.installed != lastVentoyLogFound_;
    if (!changed) {
        return;
    }

    for (const std::wstring& line : detection.logLines) {
        onLog_(line);
    }

    hasLastVentoyLog_ = true;
    lastVentoyLogDrive_ = drive;
    lastVentoyLogFound_ = detection.installed;
}

void Gui::LogDriveListSelection(const std::vector<DriveInfo>& drives, const int selectedIdx,
                                const std::wstring& previous, const int restoreIdx) {
    if (!onLog_) {
        return;
    }

    onLog_(L"Drive list refresh (show all drives: " +
           std::wstring(ShowAllDrivesChecked() ? L"yes" : L"no") + L"):");

    if (drives.empty()) {
        onLog_(L"  no eligible drives (USB/VHD/fixed >= 30 GiB; C: hidden)");
        onLog_(L"  default selection: none");
        return;
    }

    for (size_t i = 0; i < drives.size(); ++i) {
        onLog_(L"  [" + std::to_wstring(i) + L"] " + drives[i].letter + L" — " + drives[i].display);
    }

    if (selectedIdx < 0 || static_cast<size_t>(selectedIdx) >= drives.size()) {
        onLog_(L"  default selection: none");
        return;
    }

    const DriveInfo& selected = drives[static_cast<size_t>(selectedIdx)];
    std::wstring reason;
    if (restoreIdx >= 0 && !previous.empty()) {
        reason = L"kept previous selection " + previous;
    } else {
        const int vhdDefault = DefaultDriveIndex(drives);
        if (vhdDefault == selectedIdx) {
            reason = L"first VHD/VHDX in list (installer default)";
        } else if (selectedIdx == 0) {
            reason = L"first eligible drive in list (no VHD/VHDX default)";
        } else {
            reason = L"drive index " + std::to_wstring(selectedIdx);
        }
    }

    onLog_(L"  default selection: " + selected.letter + L" — " + reason);
}

void Gui::SetDownloadControlsEnabled(const bool enabled) {
    for (HWND control : {downloadMirror1Btn_, downloadMirror2Btn_, altDownloadCombo_, altDownloadOpenBtn_}) {
        if (control && IsWindow(control)) {
            EnableWindow(control, enabled);
        }
    }
}

void Gui::SetBusy(const bool busy, const BusyProgressMode progressMode) {
    EnableWindow(installBtn_, !busy && !archiveMissing_);
    EnableWindow(verifyFilesBtn_, !busy);
    EnableWindow(driveCombo_, !busy);
    EnableWindow(languageCombo_, !busy);
    EnableWindow(showAllDrivesCheck_, !busy);
    EnableWindow(formatCheck_, !busy);
    EnableWindow(ventoyActionCheck_, !busy);
    EnableWindow(advancedCheck_, !busy);
    SetDownloadControlsEnabled(!busy);
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
        if (pendingDriveRefresh_) {
            pendingDriveRefresh_ = false;
            RefreshDrives(true);
        } else {
            RefreshDriveVentoyStatus();
        }
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
        ClearStatusBar();
    }

    progressPercentValue_ = percent;
    progressPercentText_ = std::to_wstring(percent) + L"%";
    InvalidateRect(progressBar_, nullptr, FALSE);

    if (!newFiles.empty()) {
        SetStatusBar(i18n::Tr(L"status.extracting_file", std::to_wstring(percent),
                              ShortDisplayPath(newFiles.back())));
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

void Gui::SetDownloadProgress(const int percent, const std::wstring& barText, const std::wstring& labelText) {
    progressPercentValue_ = percent;
    progressPercentText_ = barText.empty() ? (std::to_wstring(percent) + L"%") : barText;
    InvalidateRect(progressBar_, nullptr, FALSE);
    SetStatusBar(labelText);
}

void Gui::StartMirrorDownload(const std::wstring& url, const std::wstring& mirrorName) {
    if (downloadingArchive_.exchange(true)) {
        return;
    }

    const std::wstring destination = JoinPath(GetExeDirectory(), kMediCatArchiveFileName);
    const std::wstring tempPath = destination + L".part";
    const uint64_t partialSize = GetFileSizeBytes(tempPath);
    const bool resuming = partialSize > 0;

    SetBusy(true, BusyProgressMode::Download);
    SetDownloadProgress(
        0, FormatProgressBytes(partialSize),
        i18n::Tr(resuming ? L"status.resuming_archive_mirror" : L"status.downloading_archive_mirror", mirrorName));

    HWND hwnd = hwnd_;

    std::thread([this, hwnd, url, mirrorName, destination, tempPath, resuming, partialSize]() {
        std::wstring netError;
        if (!TestInternetConnection(netError)) {
            downloadingArchive_ = false;
            auto* payload = new DonePayload{false, i18n::Tr(L"messages.no_internet"), i18n::Tr(L"titles.download_failed")};
            PostMessageW(hwnd, WM_MEDICAT_DONE, 0, reinterpret_cast<LPARAM>(payload));
            return;
        }

        uint64_t lastUiTick = 0;
        uint64_t lastDownloaded = 0;
        bool showResuming = resuming;
        std::wstring error;
        const bool ok = HttpDownloadFileWithProgress(
            url, tempPath,
            [&](const uint64_t downloaded, const uint64_t total) {
                const uint64_t now = GetTickCount64();
                if (lastUiTick != 0 && now - lastUiTick < 200) {
                    return;
                }

                uint64_t speed = 0;
                if (lastUiTick != 0 && now > lastUiTick && downloaded >= lastDownloaded) {
                    const uint64_t deltaBytes = downloaded - lastDownloaded;
                    const uint64_t deltaMs = now - lastUiTick;
                    speed = (deltaBytes * 1000) / deltaMs;
                }
                lastUiTick = now;
                lastDownloaded = downloaded;

                if (showResuming && downloaded > partialSize) {
                    showResuming = false;
                }

                const int percent = DownloadPercent(downloaded, total);
                const std::wstring sizeText = FormatDownloadSizeText(downloaded, total);
                std::wstring statusLine = i18n::Tr(showResuming ? L"status.resuming_archive_mirror"
                                                                  : L"status.downloading_archive_mirror",
                                                   mirrorName);
                const std::wstring speedText = FormatDownloadSpeed(speed);
                if (!speedText.empty()) {
                    statusLine += L" · ";
                    statusLine += speedText;
                }

                auto* progress = new ProgressPayload{};
                progress->downloadUpdate = true;
                progress->percent = percent;
                progress->statusText = sizeText;
                progress->file = statusLine;
                PostMessageW(hwnd, WM_MEDICAT_PROGRESS, 0, reinterpret_cast<LPARAM>(progress));
            },
            error);

        if (!ok) {
            downloadingArchive_ = false;
            auto* payload =
                new DonePayload{false, i18n::Tr(L"messages.archive_download_failed", error), i18n::Tr(L"titles.download_failed")};
            PostMessageW(hwnd, WM_MEDICAT_DONE, 0, reinterpret_cast<LPARAM>(payload));
            return;
        }

        DeleteFileW(destination.c_str());
        if (!MoveFileW(tempPath.c_str(), destination.c_str())) {
            DeleteFileW(tempPath.c_str());
            downloadingArchive_ = false;
            auto* payload = new DonePayload{false, i18n::Tr(L"messages.archive_download_failed", L"Could not save archive file"),
                                            i18n::Tr(L"titles.download_failed")};
            PostMessageW(hwnd, WM_MEDICAT_DONE, 0, reinterpret_cast<LPARAM>(payload));
            return;
        }

        downloadingArchive_ = false;
        auto* payload = new DonePayload{true, L"", L""};
        PostMessageW(hwnd, WM_MEDICAT_DONE, 0, reinterpret_cast<LPARAM>(payload));
    }).detach();
}

void Gui::SetStatusBar(const std::wstring& text) {
    if (statusBar_ && IsWindow(statusBar_)) {
        SetWindowTextW(statusBar_, text.c_str());
    }
}

void Gui::ClearStatusBar() {
    SetStatusBar(L"");
}

void Gui::ClearFileLog() {
    {
        std::lock_guard lock(uiMutex_);
        fileLogLines_.clear();
        fileLogDisplayLines_.clear();
    }
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

void Gui::RefreshCreditsWindowText() {
    if (!creditsWindow_ || !IsWindow(creditsWindow_)) {
        return;
    }

    SetWindowTextW(creditsWindow_, i18n::Tr(L"ui.credits_window_title").c_str());
    if (creditsIntro_ && IsWindow(creditsIntro_)) {
        SetWindowTextW(creditsIntro_, i18n::Tr(L"ui.credits_intro").c_str());
    }
    if (creditsSevenZipBtn_ && IsWindow(creditsSevenZipBtn_)) {
        SetWindowTextW(creditsSevenZipBtn_, i18n::Tr(L"ui.credits_open_7zip").c_str());
    }
    if (creditsVentoyBtn_ && IsWindow(creditsVentoyBtn_)) {
        SetWindowTextW(creditsVentoyBtn_, i18n::Tr(L"ui.credits_open_ventoy").c_str());
    }
    if (creditsCloseBtn_ && IsWindow(creditsCloseBtn_)) {
        SetWindowTextW(creditsCloseBtn_, i18n::Tr(L"ui.credits_close_button").c_str());
    }
    LayoutCreditsWindow();
}

void Gui::LayoutCreditsWindow() {
    if (!creditsWindow_ || !IsWindow(creditsWindow_)) {
        return;
    }

    RECT client{};
    GetClientRect(creditsWindow_, &client);
    const int clientWidth = client.right - client.left;
    const int contentWidth = std::max(0, clientWidth - 2 * kCreditsDialogMargin);

    int y = kCreditsDialogTopPad;
    if (creditsIntro_ && IsWindow(creditsIntro_)) {
        const std::wstring intro = i18n::Tr(L"ui.credits_intro");
        const int introHeight = MeasureWrappedStaticHeight(creditsIntro_, intro, contentWidth);
        SetWindowPos(creditsIntro_, nullptr, kCreditsDialogMargin, y, contentWidth, introHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        y += introHeight + kCreditsDialogSectionGap;
    }

    const auto placeButton = [&](HWND button) {
        if (!button || !IsWindow(button)) {
            return;
        }
        SetWindowPos(button, nullptr, kCreditsDialogMargin, y, contentWidth, kCreditsDialogBtnHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        y += kCreditsDialogBtnHeight + kCreditsDialogBtnGap;
    };

    placeButton(creditsSevenZipBtn_);
    placeButton(creditsVentoyBtn_);
    y += kCreditsDialogBtnGap;
    placeButton(creditsCloseBtn_);
}

void Gui::OpenCreditsWindow() {
    if (creditsWindow_ && IsWindow(creditsWindow_)) {
        RefreshCreditsWindowText();
        ShowWindow(creditsWindow_, SW_SHOW);
        SetForegroundWindow(creditsWindow_);
        return;
    }

    RECT mainRc{};
    GetWindowRect(hwnd_, &mainRc);
    const int windowWidth = CreditsOuterWidth();
    const int windowHeight = CreditsOuterHeight();
    const int x = mainRc.left + ((mainRc.right - mainRc.left) - windowWidth) / 2;
    const int y = mainRc.top + ((mainRc.bottom - mainRc.top) - windowHeight) / 2;

    creditsWindow_ = CreateWindowExW(
        0, kCreditsWindowClass, i18n::Tr(L"ui.credits_window_title").c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, windowWidth, windowHeight, hwnd_, nullptr, instance_, this);
    if (!creditsWindow_) {
        return;
    }

    theme::EnableDarkModeRecursive(creditsWindow_);
    const HFONT uiFont = theme::MakeUiFont();
    const HFONT subtitleFont = theme::MakeSubtitleFont();

    creditsIntro_ = CreateWindowW(
        L"STATIC", i18n::Tr(L"ui.credits_intro").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        0, 0, 100, 40, creditsWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCreditsIntroId)), instance_, nullptr);

    creditsSevenZipBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.credits_open_7zip").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 100, kCreditsDialogBtnHeight, creditsWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCreditsSevenZipBtnId)), instance_, nullptr);

    creditsVentoyBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.credits_open_ventoy").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 100, kCreditsDialogBtnHeight, creditsWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCreditsVentoyBtnId)), instance_, nullptr);

    creditsCloseBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.credits_close_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 100, kCreditsDialogBtnHeight, creditsWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCreditsCloseBtnId)), instance_, nullptr);

    SendMessageW(creditsIntro_, WM_SETFONT, reinterpret_cast<WPARAM>(subtitleFont), TRUE);
    for (HWND child : {creditsSevenZipBtn_, creditsVentoyBtn_, creditsCloseBtn_}) {
        if (child && IsWindow(child)) {
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        }
    }

    SubclassGlowButton(creditsSevenZipBtn_, false);
    SubclassGlowButton(creditsVentoyBtn_, false);
    SubclassGlowButton(creditsCloseBtn_, false);

    LayoutCreditsWindow();
    ShowWindow(creditsWindow_, SW_SHOW);
    SetForegroundWindow(creditsWindow_);
    UpdateWindow(creditsWindow_);
}

LRESULT CALLBACK Gui::CreditsWndProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
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
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            if (id == kCreditsSevenZipBtnId) {
                OpenBrowserUrl(kSevenZipProjectUrl);
                return 0;
            }
            if (id == kCreditsVentoyBtnId) {
                OpenBrowserUrl(kVentoyProjectUrl);
                return 0;
            }
            if (id == kCreditsCloseBtnId) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, theme::GetBrushes().window);
            return 1;
        }
        case WM_SIZE:
            self->LayoutCreditsWindow();
            return 0;
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            const HWND ctl = reinterpret_cast<HWND>(lp);
            SetBkMode(hdc, TRANSPARENT);
            if (ctl == self->creditsIntro_) {
                SetTextColor(hdc, theme::Colors().muted);
            } else {
                SetTextColor(hdc, theme::Colors().text);
            }
            return reinterpret_cast<LRESULT>(theme::GetBrushes().window);
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            self->creditsWindow_ = nullptr;
            self->creditsIntro_ = nullptr;
            self->creditsSevenZipBtn_ = nullptr;
            self->creditsVentoyBtn_ = nullptr;
            self->creditsCloseBtn_ = nullptr;
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void Gui::FinishReExtractPrompt(const bool wantReExtract) {
    if (!activeReExtractPrompt_) {
        return;
    }

    bool expected = false;
    if (!activeReExtractPrompt_->completed.compare_exchange_strong(expected, true)) {
        return;
    }

    activeReExtractPrompt_->wantReExtract = wantReExtract;
    if (activeReExtractPrompt_->doneEvent) {
        SetEvent(activeReExtractPrompt_->doneEvent);
    }

    const HWND window = reExtractWindow_;
    activeReExtractPrompt_.reset();
    if (window && IsWindow(window)) {
        DestroyWindow(window);
    }
}

void Gui::LayoutReExtractDialog() {
    if (!reExtractWindow_ || !IsWindow(reExtractWindow_)) {
        return;
    }

    RECT client{};
    GetClientRect(reExtractWindow_, &client);
    const int clientWidth = client.right - client.left;
    const int contentWidth = std::max(0, clientWidth - 2 * kReExtractDialogMargin);
    const int reExtractBtnWidth = 220;
    const int closeBtnWidth = 120;
    const int actionGap = kReExtractDialogBtnGap;
    const int actionRowWidth = reExtractBtnWidth + actionGap + closeBtnWidth;

    int y = kMsgDialogAccentHeight + kReExtractDialogTopPad;
    if (reExtractMessage_ && IsWindow(reExtractMessage_)) {
        const int messageHeight = reExtractMessageHeight_ > 0 ? reExtractMessageHeight_ : kReExtractMaxMessageHeight;
        LayoutScrollableTextPane(reExtractMessage_, kReExtractDialogMargin, y, contentWidth, messageHeight);
        y += messageHeight + kReExtractDialogSectionGap;
    }

    if (reExtractList_ && IsWindow(reExtractList_)) {
        const int clientHeight = client.bottom - client.top;
        const int buttonsBlock = kReExtractDialogBtnHeight + 16;
        const int listHeight = std::max(kReExtractMinListHeight, clientHeight - y - kReExtractDialogSectionGap - buttonsBlock);
        SetWindowPos(reExtractList_, nullptr, kReExtractDialogMargin, y, contentWidth, listHeight, SWP_NOZORDER);
        y += listHeight + kReExtractDialogSectionGap;
    }

    const int actionX = kReExtractDialogMargin + std::max(0, contentWidth - actionRowWidth);
    if (reExtractBtn_ && IsWindow(reExtractBtn_)) {
        SetWindowPos(reExtractBtn_, nullptr, actionX, y, reExtractBtnWidth, kReExtractDialogBtnHeight, SWP_NOZORDER);
    }
    if (reExtractCloseBtn_ && IsWindow(reExtractCloseBtn_)) {
        SetWindowPos(reExtractCloseBtn_, nullptr, actionX + reExtractBtnWidth + actionGap, y, closeBtnWidth,
                     kReExtractDialogBtnHeight, SWP_NOZORDER);
    }
}

void Gui::OpenReExtractPrompt(ReExtractPromptPayload* payload) {
    if (!payload || !payload->state || !payload->state->doneEvent) {
        delete payload;
        return;
    }

    if (reExtractWindow_ && IsWindow(reExtractWindow_)) {
        DestroyWindow(reExtractWindow_);
    }

    activeReExtractPrompt_ = payload->state;
    reExtractFailureLines_ = payload->failures;
    reExtractFailedFilesTotal_ = payload->failedFiles > 0 ? payload->failedFiles : payload->failures.size();

    const std::wstring title =
        payload->title.empty() ? i18n::Tr(L"ui.re_extract_window_title") : payload->title;
    const std::wstring summary = i18n::Tr(L"ui.re_extract_summary", std::to_wstring(reExtractFailedFilesTotal_));
    std::wstring bodyText = payload->message;
    if (!bodyText.empty() && !summary.empty()) {
        bodyText += L"\r\n\r\n";
    }
    bodyText += summary;

    const HFONT uiFont = theme::MakeUiFont();
    const int contentWidth = kReExtractClientWidth - 2 * kReExtractDialogMargin;
    int measuredMessageHeight = 64;
    {
        const HWND measureHost = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, instance_,
                                                 nullptr);
        if (measureHost) {
            SendMessageW(measureHost, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
            measuredMessageHeight = MeasureWrappedStaticHeight(measureHost, bodyText, contentWidth);
            DestroyWindow(measureHost);
        }
    }

    const size_t listRows = reExtractFailureLines_.size() +
                            (reExtractFailedFilesTotal_ > reExtractFailureLines_.size() ? 1U : 0U);
    const int listHeight = std::clamp(static_cast<int>(listRows) * kReExtractRowHeight + 12, kReExtractMinListHeight,
                                      kReExtractMaxListHeight);
    reExtractMessageHeight_ = std::min(measuredMessageHeight, kReExtractMaxMessageHeight);

    const int windowWidth = ReExtractOuterWidth();
    const int windowHeight = ReExtractOuterHeightForContent(measuredMessageHeight, listHeight);
    const int x = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
    const int yPos = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

    reExtractWindow_ = CreateWindowExW(
        0, kReExtractWindowClass, title.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, yPos, windowWidth, windowHeight, hwnd_, nullptr, instance_, this);
    if (!reExtractWindow_) {
        FinishReExtractPrompt(false);
        delete payload;
        return;
    }

    theme::EnableDarkModeRecursive(reExtractWindow_);

    reExtractMessage_ = CreateScrollableTextPane(reExtractWindow_, instance_, kReExtractMessageId, uiFont, bodyText);

    reExtractList_ = CreateWindowW(
        L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
        0, 0, 100, listHeight, reExtractWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kReExtractListId)), instance_, nullptr);

    reExtractBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.re_extract_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        0, 0, 100, kReExtractDialogBtnHeight, reExtractWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kReExtractBtnId)), instance_, nullptr);

    reExtractCloseBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.re_extract_close_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        0, 0, 100, kReExtractDialogBtnHeight, reExtractWindow_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kReExtractCloseBtnId)), instance_, nullptr);

    const HFONT logFont = theme::MakeLogFont();
    for (HWND child : {reExtractList_, reExtractBtn_, reExtractCloseBtn_}) {
        if (child && IsWindow(child)) {
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        }
    }
    SendMessageW(reExtractList_, WM_SETFONT, reinterpret_cast<WPARAM>(logFont), TRUE);

    SendMessageW(reExtractList_, LB_RESETCONTENT, 0, 0);
    for (const std::wstring& line : reExtractFailureLines_) {
        SendMessageW(reExtractList_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }
    if (reExtractFailedFilesTotal_ > reExtractFailureLines_.size()) {
        const std::wstring more = L"... and " +
                                  std::to_wstring(reExtractFailedFilesTotal_ - reExtractFailureLines_.size()) +
                                  L" more";
        SendMessageW(reExtractList_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(more.c_str()));
    }
    SendMessageW(reExtractList_, LB_SETHORIZONTALEXTENT, 8000, 0);

    SubclassGlowButton(reExtractBtn_, true);
    SubclassGlowButton(reExtractCloseBtn_, false);

    delete payload;

    LayoutReExtractDialog();
    ShowWindow(reExtractWindow_, SW_SHOW);
    SetForegroundWindow(reExtractWindow_);
    UpdateWindow(reExtractWindow_);
}

LRESULT CALLBACK Gui::ReExtractWndProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
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
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            if (id == kReExtractBtnId) {
                self->FinishReExtractPrompt(true);
                return 0;
            }
            if (id == kReExtractCloseBtnId) {
                self->FinishReExtractPrompt(false);
                return 0;
            }
            break;
        }
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            RECT accent = rc;
            accent.bottom = accent.top + kMsgDialogAccentHeight;
            HBRUSH accentBrush = CreateSolidBrush(self->MessageDialogAccentColor(MessageDialogKind::Warning));
            FillRect(hdc, &accent, accentBrush);
            DeleteObject(accentBrush);
            rc.top = accent.bottom;
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
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, theme::Colors().control);
            SetTextColor(hdc, theme::Colors().text);
            return reinterpret_cast<LRESULT>(theme::GetBrushes().control);
        }
        case WM_SIZE:
            self->LayoutReExtractDialog();
            return 0;
        case WM_CLOSE:
            self->FinishReExtractPrompt(false);
            return 0;
        case WM_DESTROY:
            self->reExtractWindow_ = nullptr;
            self->reExtractMessage_ = nullptr;
            self->reExtractList_ = nullptr;
            self->reExtractBtn_ = nullptr;
            self->reExtractCloseBtn_ = nullptr;
            self->reExtractFailureLines_.clear();
            self->reExtractFailedFilesTotal_ = 0;
            self->reExtractMessageHeight_ = 0;
            if (self->activeReExtractPrompt_ && !self->activeReExtractPrompt_->completed.load()) {
                self->FinishReExtractPrompt(false);
            }
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

std::wstring Gui::FailureDiagDisplayText() const {
    if (failureDiagResolved_ && failureDiagUploadSucceeded_ && !failureDiagKeyword_.empty()) {
        return failureDiagKeyword_;
    }
    if (failureDiagResolved_) {
        return i18n::Tr(L"messages.diag_unavailable");
    }
    return i18n::Tr(L"messages.diag_uploading");
}

void Gui::ResetFailureDiagCode() {
    failureDiagUploadSucceeded_ = false;
    failureDiagKeyword_.clear();
    failureDiagResolved_ = false;
}

void Gui::SetFailureDiagCode(const bool uploadSucceeded, const std::wstring& keyword) {
    failureDiagUploadSucceeded_ = uploadSucceeded;
    failureDiagKeyword_ = keyword;
    failureDiagResolved_ = true;
    if (messageDialogDiagEdit_ && IsWindow(messageDialogDiagEdit_)) {
        SetWindowTextW(messageDialogDiagEdit_, FailureDiagDisplayText().c_str());
        SendMessageW(messageDialogDiagEdit_, EM_SETSEL, 0, -1);
    }
}

bool Gui::CopyFailureDiagCodeToClipboard() {
    if (!messageDialogDiagEdit_ || !IsWindow(messageDialogDiagEdit_)) {
        return false;
    }
    if (!failureDiagUploadSucceeded_ || failureDiagKeyword_.empty()) {
        return false;
    }
    return CopyTextToClipboard(messageDialog_, failureDiagKeyword_);
}

COLORREF Gui::MessageDialogAccentColor(const MessageDialogKind kind) const {
    switch (kind) {
        case MessageDialogKind::Info:
            return theme::Colors().accent;
        case MessageDialogKind::Warning:
            return RGB(230, 176, 60);
        case MessageDialogKind::Error:
            return RGB(220, 82, 82);
        default:
            return theme::Colors().accent;
    }
}

void Gui::LayoutMessageDialog() {
    if (!messageDialog_ || !IsWindow(messageDialog_)) {
        return;
    }

    const bool showDiag = messageDialogFooter_ == MessageDialogFooter::OkWithDiag;
    const bool showYesNo = messageDialogFooter_ == MessageDialogFooter::YesNo;

    RECT client{};
    GetClientRect(messageDialog_, &client);
    const int clientWidth = client.right - client.left;
    const int contentWidth = std::max(0, clientWidth - 2 * kMsgDialogMargin);
    const int copyBtnWidth = 100;
    const int discordBtnWidth = 100;
    const int okBtnWidth = 100;
    const int yesBtnWidth = 100;
    const int noBtnWidth = 100;
    const int actionGap = kMsgDialogBtnGap;

    auto setVisible = [](HWND control, const int show) {
        if (control && IsWindow(control)) {
            ShowWindow(control, show);
        }
    };

    setVisible(messageDialogDiagLabel_, showDiag ? SW_SHOW : SW_HIDE);
    setVisible(messageDialogDiagEdit_, showDiag ? SW_SHOW : SW_HIDE);
    setVisible(messageDialogCopyBtn_, showDiag ? SW_SHOW : SW_HIDE);
    setVisible(messageDialogDiscordBtn_, showDiag ? SW_SHOW : SW_HIDE);
    setVisible(messageDialogOkBtn_, showYesNo ? SW_HIDE : SW_SHOW);
    setVisible(messageDialogYesBtn_, showYesNo ? SW_SHOW : SW_HIDE);
    setVisible(messageDialogNoBtn_, showYesNo ? SW_SHOW : SW_HIDE);

    int y = kMsgDialogAccentHeight + kMsgDialogTopPad;
    if (messageDialogBody_ && IsWindow(messageDialogBody_)) {
        const int messageHeight =
            messageDialogBodyHeight_ > 0 ? messageDialogBodyHeight_ : kMsgDialogMaxMessageHeight;
        LayoutScrollableTextPane(messageDialogBody_, kMsgDialogMargin, y, contentWidth, messageHeight);
        y += messageHeight + kMsgDialogSectionGap;
    }

    if (showDiag && messageDialogDiagLabel_ && IsWindow(messageDialogDiagLabel_)) {
        const int labelHeight = 18;
        SetWindowPos(messageDialogDiagLabel_, nullptr, kMsgDialogMargin, y, contentWidth, labelHeight, SWP_NOZORDER);
        y += labelHeight + 6;
    }

    if (showDiag && messageDialogDiagEdit_ && IsWindow(messageDialogDiagEdit_)) {
        const int sideBtnWidth = copyBtnWidth + actionGap + discordBtnWidth;
        const int editWidth = std::max(120, contentWidth - sideBtnWidth - actionGap);
        SetWindowPos(messageDialogDiagEdit_, nullptr, kMsgDialogMargin, y, editWidth, kMsgDialogDiagEditHeight,
                     SWP_NOZORDER);
        const int sideBtnX = kMsgDialogMargin + editWidth + actionGap;
        if (messageDialogCopyBtn_ && IsWindow(messageDialogCopyBtn_)) {
            SetWindowPos(messageDialogCopyBtn_, nullptr, sideBtnX, y, copyBtnWidth, kMsgDialogBtnHeight,
                         SWP_NOZORDER);
        }
        if (messageDialogDiscordBtn_ && IsWindow(messageDialogDiscordBtn_)) {
            SetWindowPos(messageDialogDiscordBtn_, nullptr, sideBtnX + copyBtnWidth + actionGap, y, discordBtnWidth,
                         kMsgDialogBtnHeight, SWP_NOZORDER);
        }
        y += kMsgDialogDiagEditHeight + kMsgDialogSectionGap;
    }

    if (showDiag) {
        const int actionX = kMsgDialogMargin + std::max(0, contentWidth - okBtnWidth);
        if (messageDialogOkBtn_ && IsWindow(messageDialogOkBtn_)) {
            SetWindowPos(messageDialogOkBtn_, nullptr, actionX, y, okBtnWidth, kMsgDialogBtnHeight, SWP_NOZORDER);
        }
    } else if (showYesNo) {
        const int actionRowWidth = noBtnWidth + actionGap + yesBtnWidth;
        const int actionX = kMsgDialogMargin + std::max(0, contentWidth - actionRowWidth);
        if (messageDialogNoBtn_ && IsWindow(messageDialogNoBtn_)) {
            SetWindowPos(messageDialogNoBtn_, nullptr, actionX, y, noBtnWidth, kMsgDialogBtnHeight, SWP_NOZORDER);
        }
        if (messageDialogYesBtn_ && IsWindow(messageDialogYesBtn_)) {
            SetWindowPos(messageDialogYesBtn_, nullptr, actionX + noBtnWidth + actionGap, y, yesBtnWidth,
                         kMsgDialogBtnHeight, SWP_NOZORDER);
        }
    } else if (messageDialogOkBtn_ && IsWindow(messageDialogOkBtn_)) {
        const int actionX = kMsgDialogMargin + std::max(0, contentWidth - okBtnWidth);
        SetWindowPos(messageDialogOkBtn_, nullptr, actionX, y, okBtnWidth, kMsgDialogBtnHeight, SWP_NOZORDER);
    }
}

void Gui::RefreshMessageDialogText() {
    if (!messageDialog_ || !IsWindow(messageDialog_)) {
        return;
    }
    if (messageDialogDiagLabel_ && IsWindow(messageDialogDiagLabel_)) {
        SetWindowTextW(messageDialogDiagLabel_, i18n::Tr(L"ui.failure_dialog_diag_label").c_str());
    }
    if (messageDialogCopyBtn_ && IsWindow(messageDialogCopyBtn_)) {
        SetWindowTextW(messageDialogCopyBtn_, i18n::Tr(L"ui.failure_dialog_copy_button").c_str());
    }
    if (messageDialogOkBtn_ && IsWindow(messageDialogOkBtn_)) {
        SetWindowTextW(messageDialogOkBtn_, i18n::Tr(L"ui.failure_dialog_ok_button").c_str());
    }
    if (messageDialogYesBtn_ && IsWindow(messageDialogYesBtn_)) {
        SetWindowTextW(messageDialogYesBtn_, i18n::Tr(L"ui.dialog_yes").c_str());
    }
    if (messageDialogNoBtn_ && IsWindow(messageDialogNoBtn_)) {
        SetWindowTextW(messageDialogNoBtn_, i18n::Tr(L"ui.dialog_no").c_str());
    }
    if (messageDialogDiagEdit_ && IsWindow(messageDialogDiagEdit_)) {
        SetWindowTextW(messageDialogDiagEdit_, FailureDiagDisplayText().c_str());
    }
    LayoutMessageDialog();
}

bool Gui::RunMessageDialogModalLoop() {
    if (!messageDialog_ || !IsWindow(messageDialog_)) {
        return false;
    }

    messageDialogModalActive_ = true;
    messageDialogModalResult_ = false;

    HWND parent = hwnd_;
    if (parent && IsWindow(parent)) {
        EnableWindow(parent, FALSE);
    }

    MSG msg{};
    while (messageDialogModalActive_ && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_QUIT) {
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }
        if (!IsDialogMessageW(messageDialog_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (parent && IsWindow(parent)) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }

    const bool result = messageDialogModalResult_;
    messageDialogModalActive_ = false;
    return result;
}

void Gui::OpenMessageDialogInternal(const std::wstring& message, const std::wstring& title,
                                    const MessageDialogKind kind, const MessageDialogFooter footer,
                                    const bool modal) {
    if (messageDialog_ && IsWindow(messageDialog_)) {
        DestroyWindow(messageDialog_);
    }

    messageDialogKind_ = kind;
    messageDialogFooter_ = footer;
    messageDialogModal_ = modal;

    const HFONT uiFont = theme::MakeUiFont();
    const HFONT logFont = theme::MakeLogFont();

    const int contentWidth = kMsgDialogClientWidth - 2 * kMsgDialogMargin;
    int measuredMessageHeight = 120;
    {
        const HWND measureHost = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, instance_,
                                                 nullptr);
        if (measureHost) {
            SendMessageW(measureHost, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
            measuredMessageHeight = MeasureWrappedStaticHeight(measureHost, message, contentWidth);
            DestroyWindow(measureHost);
        }
    }

    messageDialogBodyHeight_ = std::min(measuredMessageHeight, kMsgDialogMaxMessageHeight);

    const int windowWidth = MessageDialogOuterWidth();
    const int windowHeight = MessageDialogOuterHeightForMessage(measuredMessageHeight, footer);
    const int x = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
    const int yPos = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

    messageDialog_ = CreateWindowExW(
        0, kMessageDialogWindowClass, title.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, yPos, windowWidth, windowHeight, hwnd_, nullptr, instance_, this);
    if (!messageDialog_) {
        MessageBoxW(hwnd_, message.c_str(), title.c_str(), MB_ICONERROR);
        return;
    }

    theme::EnableDarkModeRecursive(messageDialog_);

    messageDialogBody_ =
        CreateScrollableTextPane(messageDialog_, instance_, kMsgDialogMessageId, uiFont, message);

    messageDialogDiagLabel_ = CreateWindowW(
        L"STATIC", i18n::Tr(L"ui.failure_dialog_diag_label").c_str(), WS_CHILD | SS_LEFT, 0, 0, 100, 18,
        messageDialog_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMsgDialogDiagLabelId)), instance_, nullptr);

    messageDialogDiagEdit_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", FailureDiagDisplayText().c_str(),
        WS_CHILD | ES_AUTOHSCROLL | ES_READONLY | WS_TABSTOP, 0, 0, 100, kMsgDialogDiagEditHeight, messageDialog_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMsgDialogDiagEditId)), instance_, nullptr);

    messageDialogCopyBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.failure_dialog_copy_button").c_str(), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0,
        100, kMsgDialogBtnHeight, messageDialog_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMsgDialogCopyBtnId)), instance_, nullptr);

    messageDialogDiscordBtn_ = CreateWindowW(
        L"BUTTON", L"Discord", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 100, kMsgDialogBtnHeight,
        messageDialog_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMsgDialogDiscordBtnId)), instance_, nullptr);

    messageDialogOkBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.failure_dialog_ok_button").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 100, kMsgDialogBtnHeight, messageDialog_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMsgDialogOkBtnId)), instance_, nullptr);

    messageDialogYesBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.dialog_yes").c_str(), WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 100,
        kMsgDialogBtnHeight, messageDialog_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMsgDialogYesBtnId)), instance_, nullptr);

    messageDialogNoBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.dialog_no").c_str(), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 100,
        kMsgDialogBtnHeight, messageDialog_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMsgDialogNoBtnId)), instance_, nullptr);

    SendMessageW(messageDialogDiagLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
    SendMessageW(messageDialogDiagEdit_, WM_SETFONT, reinterpret_cast<WPARAM>(logFont), TRUE);
    for (HWND child :
         {messageDialogCopyBtn_, messageDialogDiscordBtn_, messageDialogOkBtn_, messageDialogYesBtn_,
          messageDialogNoBtn_}) {
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
    }

    SubclassGlowButton(messageDialogCopyBtn_, false);
    SubclassGlowButton(messageDialogDiscordBtn_, false);
    SubclassGlowButton(messageDialogOkBtn_, true);
    SubclassGlowButton(messageDialogYesBtn_, true);
    SubclassGlowButton(messageDialogNoBtn_, false);

    RefreshMessageDialogText();
    ShowWindow(messageDialog_, SW_SHOW);
    SetForegroundWindow(messageDialog_);
    if (footer == MessageDialogFooter::YesNo && messageDialogNoBtn_ && IsWindow(messageDialogNoBtn_)) {
        SetFocus(messageDialogNoBtn_);
    }
    UpdateWindow(messageDialog_);
}

void Gui::ShowMessageDialog(const std::wstring& message, const std::wstring& title, const MessageDialogKind kind) {
    OpenMessageDialogInternal(message, title, kind, MessageDialogFooter::Ok, false);
}

bool Gui::ShowConfirmDialog(const std::wstring& message, const std::wstring& title, const MessageDialogKind kind) {
    OpenMessageDialogInternal(message, title, kind, MessageDialogFooter::YesNo, true);
    return RunMessageDialogModalLoop();
}

void Gui::OpenFailureDialog(const std::wstring& message, const std::wstring& title) {
    OpenMessageDialogInternal(message, title, MessageDialogKind::Error, MessageDialogFooter::OkWithDiag, false);
}

void Gui::LayoutHelpGateDialog() {
    if (!helpGateDialog_ || !IsWindow(helpGateDialog_)) {
        return;
    }

    RECT client{};
    GetClientRect(helpGateDialog_, &client);
    const int clientWidth = client.right - client.left;
    const int contentWidth = std::max(0, clientWidth - 2 * kMsgDialogMargin);
    const int continueBtnWidth = 160;

    int y = kMsgDialogAccentHeight + kMsgDialogTopPad;
    if (helpGateBody_ && IsWindow(helpGateBody_)) {
        const int messageHeight = helpGateBodyHeight_ > 0 ? helpGateBodyHeight_ : kHelpGateMaxMessageHeight;
        LayoutScrollableTextPane(helpGateBody_, kMsgDialogMargin, y, contentWidth, messageHeight);
        y += messageHeight + kMsgDialogSectionGap;
    }

    if (helpGateDiscordBtn_ && IsWindow(helpGateDiscordBtn_)) {
        SetWindowPos(helpGateDiscordBtn_, nullptr, kMsgDialogMargin, y, contentWidth, kHelpGateDiscordBtnHeight,
                     SWP_NOZORDER);
        y += kHelpGateDiscordBtnHeight + kMsgDialogSectionGap;
    }

    if (helpGateAckCheckbox_ && IsWindow(helpGateAckCheckbox_)) {
        SetWindowPos(helpGateAckCheckbox_, nullptr, kMsgDialogMargin, y, contentWidth, kHelpGateAckHeight,
                     SWP_NOZORDER);
        y += kHelpGateAckHeight + kMsgDialogSectionGap;
    }

    if (helpGateContinueBtn_ && IsWindow(helpGateContinueBtn_)) {
        const int actionX = kMsgDialogMargin + std::max(0, contentWidth - continueBtnWidth);
        SetWindowPos(helpGateContinueBtn_, nullptr, actionX, y, continueBtnWidth, kMsgDialogBtnHeight, SWP_NOZORDER);
    }
}

void Gui::RefreshHelpGateText() {
    if (!helpGateDialog_ || !IsWindow(helpGateDialog_)) {
        return;
    }

    const std::wstring message = i18n::Tr(L"messages.help_gate", std::to_wstring(helpGateFailureCount_));
    if (helpGateBody_ && IsWindow(helpGateBody_)) {
        SetScrollableTextPaneText(helpGateBody_, message);
    }
    if (helpGateDiscordBtn_ && IsWindow(helpGateDiscordBtn_)) {
        SetWindowTextW(helpGateDiscordBtn_, i18n::Tr(L"ui.help_gate_discord_button").c_str());
    }
    if (helpGateAckCheckbox_ && IsWindow(helpGateAckCheckbox_)) {
        SetWindowTextW(helpGateAckCheckbox_, i18n::Tr(L"ui.help_gate_ack_checkbox").c_str());
    }
    if (helpGateContinueBtn_ && IsWindow(helpGateContinueBtn_)) {
        SetWindowTextW(helpGateContinueBtn_, i18n::Tr(L"ui.help_gate_continue_button").c_str());
    }
    LayoutHelpGateDialog();
}

bool Gui::RunHelpGateModalLoop() {
    if (!helpGateDialog_ || !IsWindow(helpGateDialog_)) {
        return false;
    }

    helpGateModalActive_ = true;
    helpGateModalResult_ = false;

    HWND parent = hwnd_;
    if (parent && IsWindow(parent)) {
        EnableWindow(parent, FALSE);
    }

    MSG msg{};
    while (helpGateModalActive_ && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_QUIT) {
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }
        if (!IsDialogMessageW(helpGateDialog_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (parent && IsWindow(parent)) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }

    const bool result = helpGateModalResult_;
    helpGateModalActive_ = false;
    return result;
}

bool Gui::ShowHelpGateDialog(const int failureCount) {
    if (helpGateDialog_ && IsWindow(helpGateDialog_)) {
        DestroyWindow(helpGateDialog_);
    }

    helpGateFailureCount_ = failureCount;
    const HFONT uiFont = theme::MakeUiFont();
    const std::wstring message = i18n::Tr(L"messages.help_gate", std::to_wstring(failureCount));
    const std::wstring title = i18n::Tr(L"titles.help_gate");

    const int contentWidth = kHelpGateClientWidth - 2 * kMsgDialogMargin;
    int measuredMessageHeight = 120;
    {
        const HWND measureHost = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, instance_,
                                                 nullptr);
        if (measureHost) {
            SendMessageW(measureHost, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
            measuredMessageHeight = MeasureWrappedStaticHeight(measureHost, message, contentWidth);
            DestroyWindow(measureHost);
        }
    }

    helpGateBodyHeight_ = std::min(measuredMessageHeight, kHelpGateMaxMessageHeight);
    const int windowWidth = HelpGateOuterWidth();
    const int windowHeight = HelpGateOuterHeightForMessage(measuredMessageHeight);
    const int x = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
    const int yPos = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

    helpGateDialog_ = CreateWindowExW(
        0, kHelpGateWindowClass, title.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, yPos, windowWidth, windowHeight, hwnd_, nullptr, instance_, this);
    if (!helpGateDialog_) {
        return false;
    }

    theme::EnableDarkModeRecursive(helpGateDialog_);
    helpGateBody_ = CreateScrollableTextPane(helpGateDialog_, instance_, kHelpGateBodyId, uiFont, message);

    helpGateDiscordBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.help_gate_discord_button").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 100, kHelpGateDiscordBtnHeight, helpGateDialog_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHelpGateDiscordBtnId)), instance_, nullptr);

    helpGateAckCheckbox_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.help_gate_ack_checkbox").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE | WS_TABSTOP, 0, 0, 100, kHelpGateAckHeight,
        helpGateDialog_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHelpGateAckCheckboxId)), instance_, nullptr);

    helpGateContinueBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.help_gate_continue_button").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 160, kMsgDialogBtnHeight, helpGateDialog_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHelpGateContinueBtnId)), instance_, nullptr);

    for (HWND child : {helpGateDiscordBtn_, helpGateAckCheckbox_, helpGateContinueBtn_}) {
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
    }

    SubclassGlowIconButton(helpGateDiscordBtn_, discordFooterIcon_, true);
    SubclassGlowButton(helpGateContinueBtn_, true);
    SubclassFlatCheckbox(helpGateAckCheckbox_);
    EnableWindow(helpGateContinueBtn_, FALSE);

    LayoutHelpGateDialog();
    ShowWindow(helpGateDialog_, SW_SHOW);
    SetForegroundWindow(helpGateDialog_);
    SetFocus(helpGateDiscordBtn_);
    UpdateWindow(helpGateDialog_);

    return RunHelpGateModalLoop();
}

LRESULT CALLBACK Gui::HelpGateWndProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
    Gui* self = reinterpret_cast<Gui*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = reinterpret_cast<Gui*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }
    if (!self) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            if (id == kHelpGateDiscordBtnId) {
                OpenBrowserUrl(kDiscordSupportUrl);
                return 0;
            }
            if (id == kHelpGateAckCheckboxId) {
                const bool checked =
                    self->helpGateAckCheckbox_ && SendMessageW(self->helpGateAckCheckbox_, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (self->helpGateContinueBtn_ && IsWindow(self->helpGateContinueBtn_)) {
                    EnableWindow(self->helpGateContinueBtn_, checked ? TRUE : FALSE);
                }
                return 0;
            }
            if (id == kHelpGateContinueBtnId) {
                self->helpGateModalResult_ = true;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            RECT accent = rc;
            accent.bottom = accent.top + kMsgDialogAccentHeight;
            HBRUSH accentBrush = CreateSolidBrush(kHelpGateAccentColor);
            FillRect(hdc, &accent, accentBrush);
            DeleteObject(accentBrush);
            rc.top = accent.bottom;
            FillRect(hdc, &rc, theme::GetBrushes().window);
            return 1;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, theme::Colors().control);
            SetTextColor(hdc, theme::Colors().text);
            return reinterpret_cast<LRESULT>(theme::GetBrushes().control);
        }
        case WM_SIZE:
            self->LayoutHelpGateDialog();
            return 0;
        case WM_CLOSE:
            self->helpGateModalResult_ = false;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            self->helpGateDialog_ = nullptr;
            self->helpGateBody_ = nullptr;
            self->helpGateDiscordBtn_ = nullptr;
            self->helpGateAckCheckbox_ = nullptr;
            self->helpGateContinueBtn_ = nullptr;
            self->helpGateBodyHeight_ = 0;
            if (self->helpGateModalActive_) {
                self->helpGateModalActive_ = false;
            }
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Gui::MessageDialogWndProc(const HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp) {
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
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            if (id == kMsgDialogCopyBtnId) {
                if (self->CopyFailureDiagCodeToClipboard()) {
                    SetWindowTextW(self->messageDialogCopyBtn_,
                                   i18n::Tr(L"ui.failure_dialog_copied_button").c_str());
                }
                return 0;
            }
            if (id == kMsgDialogDiscordBtnId) {
                OpenBrowserUrl(kDiscordSupportUrl);
                return 0;
            }
            if (id == kMsgDialogOkBtnId) {
                if (self->messageDialogModal_) {
                    self->messageDialogModalResult_ = true;
                    self->messageDialogModalActive_ = false;
                }
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == kMsgDialogYesBtnId) {
                self->messageDialogModalResult_ = true;
                self->messageDialogModalActive_ = false;
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == kMsgDialogNoBtnId) {
                self->messageDialogModalResult_ = false;
                self->messageDialogModalActive_ = false;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            RECT accent = rc;
            accent.bottom = accent.top + kMsgDialogAccentHeight;
            HBRUSH accentBrush = CreateSolidBrush(self->MessageDialogAccentColor(self->messageDialogKind_));
            FillRect(hdc, &accent, accentBrush);
            DeleteObject(accentBrush);
            rc.top = accent.bottom;
            FillRect(hdc, &rc, theme::GetBrushes().window);
            return 1;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, TRANSPARENT);
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
        case WM_SIZE:
            self->LayoutMessageDialog();
            return 0;
        case WM_CLOSE:
            if (self->messageDialogModal_) {
                self->messageDialogModalResult_ = false;
                self->messageDialogModalActive_ = false;
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            self->messageDialog_ = nullptr;
            self->messageDialogBody_ = nullptr;
            self->messageDialogDiagLabel_ = nullptr;
            self->messageDialogDiagEdit_ = nullptr;
            self->messageDialogCopyBtn_ = nullptr;
            self->messageDialogDiscordBtn_ = nullptr;
            self->messageDialogOkBtn_ = nullptr;
            self->messageDialogYesBtn_ = nullptr;
            self->messageDialogNoBtn_ = nullptr;
            self->messageDialogBodyHeight_ = 0;
            if (self->messageDialogModal_) {
                self->messageDialogModalActive_ = false;
            }
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void Gui::ShowDone(const bool success, const std::wstring& message, const std::wstring& title) {
    downloadingArchive_ = false;
    SetBusy(false);
    if (success && message.empty()) {
        const std::wstring archivePath = JoinPath(GetExeDirectory(), kMediCatArchiveFileName);
        WIN32_FILE_ATTRIBUTE_DATA info{};
        uint64_t size = 0;
        if (GetFileAttributesExW(archivePath.c_str(), GetFileExInfoStandard, &info)) {
            size = (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
        }
        SetDownloadProgress(100, FormatDownloadSizeText(size, size), i18n::Tr(L"status.download_archive_complete"));
        UpdateArchivePanel();
        return;
    }
    if (!success && message.empty()) {
        SetProgress(0);
        return;
    }
    SetProgress(success ? 100 : 0);
    const std::wstring dialogTitle =
        title.empty() ? (success ? i18n::Tr(L"titles.installation_complete") : i18n::Tr(L"titles.installation_error"))
                      : title;
    if (!success) {
        OpenFailureDialog(message, dialogTitle);
        return;
    }
    ShowMessageDialog(message, dialogTitle, MessageDialogKind::Info);
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

namespace {

bool RequiresForcedVentoyInstall(const bool ventoyOnDrive, const std::wstring& drive) {
    return !ventoyOnDrive && !drive.empty();
}

}  // namespace

bool Gui::FormatChecked() const {
    const std::wstring drive = SelectedDrive();
    if (RequiresForcedVentoyInstall(ventoyOnDrive_, drive)) {
        return true;
    }
    return SendMessageW(formatCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool Gui::RunVentoyChecked() const {
    const std::wstring drive = SelectedDrive();
    if (RequiresForcedVentoyInstall(ventoyOnDrive_, drive)) {
        return true;
    }
    return SendMessageW(ventoyActionCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool Gui::VentoyOnSelectedDrive() const {
    return ventoyOnDrive_;
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

void Gui::LayoutHeader() {
    RECT clientRect{};
    GetClientRect(hwnd_, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int contentLeft = ContentLeft(clientWidth);
    const int languageComboX = contentLeft + kContentWidth - kLanguageComboWidth - kLanguageComboInset;
    const int headerLogoX = contentLeft;
    const int headerTitleX = headerLogoX + kLogoMaxSize + 10;
    const int headerTitleWidth = languageComboX - headerTitleX - 12;

    if (logoStatic_ && IsWindow(logoStatic_)) {
        SetWindowPos(logoStatic_, nullptr, headerLogoX, kHeaderLogoY, kLogoMaxSize, kLogoMaxSize,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (titleLabel_ && IsWindow(titleLabel_)) {
        SetWindowPos(titleLabel_, nullptr, headerTitleX, kHeaderTitleY, headerTitleWidth, 28,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (versionLabel_ && IsWindow(versionLabel_)) {
        SetWindowPos(versionLabel_, nullptr, languageComboX, kVersionLabelY, kVersionLabelWidth, kVersionLabelHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (languageCombo_ && IsWindow(languageCombo_)) {
        SetWindowPos(languageCombo_, nullptr, languageComboX, kLanguageComboY, kLanguageComboWidth,
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

    LayoutMainContent();
}

bool Gui::IsArchiveAvailable() const {
    const MediCatArchiveInfo info = InspectMediCatArchive(ResolveMediCatArchivePath(GetExeDirectory()));
    return info.state == MediCatArchiveState::SizeOk || info.state == MediCatArchiveState::Verified;
}

void Gui::RefreshArchivePanelLabel() {
    if (!archiveMissingLabel_ || !IsWindow(archiveMissingLabel_)) {
        return;
    }

    const MediCatArchiveInfo info = InspectMediCatArchive(ResolveMediCatArchivePath(GetExeDirectory()));
    const wchar_t* labelKey =
        info.state == MediCatArchiveState::Incomplete ? L"ui.archive_incomplete" : L"ui.archive_missing";
    SetWindowTextW(archiveMissingLabel_, i18n::Tr(labelKey, kMediCatArchiveFileName).c_str());
}

void Gui::PopulateAlternativeDownloadCombo() {
    if (!altDownloadCombo_ || !IsWindow(altDownloadCombo_)) {
        return;
    }

    const int previous = static_cast<int>(SendMessageW(altDownloadCombo_, CB_GETCURSEL, 0, 0));
    SendMessageW(altDownloadCombo_, CB_RESETCONTENT, 0, 0);
    for (const AlternativeDownloadOption& option : kAlternativeDownloads) {
        SendMessageW(altDownloadCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(i18n::Tr(option.labelKey).c_str()));
    }

    const int count = static_cast<int>(std::size(kAlternativeDownloads));
    SendMessageW(altDownloadCombo_, CB_SETCURSEL, previous >= 0 && previous < count ? previous : 0, 0);
}

void Gui::OpenSelectedAlternativeDownload() {
    if (!altDownloadCombo_ || !IsWindow(altDownloadCombo_)) {
        return;
    }

    const int selected = static_cast<int>(SendMessageW(altDownloadCombo_, CB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(std::size(kAlternativeDownloads))) {
        return;
    }

    OpenBrowserUrl(kAlternativeDownloads[static_cast<size_t>(selected)].url);
}

void Gui::UpdateArchivePanel() {
    const MediCatArchiveInfo info = InspectMediCatArchive(ResolveMediCatArchivePath(GetExeDirectory()));
    const bool missing = info.state == MediCatArchiveState::Missing || info.state == MediCatArchiveState::Incomplete;
    if (missing == archiveMissing_) {
        if (missing) {
            RefreshArchivePanelLabel();
        }
        return;
    }

    archiveMissing_ = missing;
    RefreshArchivePanelLabel();
    const int show = missing ? SW_SHOW : SW_HIDE;
    for (HWND control :
         {archiveMissingLabel_, downloadMirror1Btn_, downloadMirror2Btn_, altDownloadCombo_, altDownloadOpenBtn_}) {
        if (control && IsWindow(control)) {
            ShowWindow(control, show);
        }
    }

    LayoutMainContent();
}

void Gui::LayoutMainContent() {
    const bool expanded = AdvancedChecked();

    int betaNoticeHeight = kBetaNoticeMinHeight;
    if (betaNoticeLabel_ && IsWindow(betaNoticeLabel_)) {
        betaNoticeHeight = MeasureWrappedStaticHeight(betaNoticeLabel_, i18n::Tr(L"ui.beta_telemetry_notice"),
                                                      kContentWidth, kBetaNoticeMinHeight);
    }

    const MainContentLayout layout = ComputeMainContentLayout(expanded, archiveMissing_, betaNoticeHeight);

    RECT clientRect{};
    GetClientRect(hwnd_, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int contentLeft = ContentLeft(clientWidth);
    const int verifyBtnX = contentLeft + kInstallBtnWidth + kActionBtnGap;
    const int progressBarWidth = kContentWidth - kOpenLogBtnWidth - kActionBtnGap;
    const int openLogBtnX = contentLeft + progressBarWidth + kActionBtnGap;
    const int mirror2X = contentLeft + kMirrorBtnWidth + kDownloadBtnGap;
    const int altOpenBtnX = contentLeft + kAltComboWidth + kDownloadBtnGap;

    if (archiveMissing_) {
        if (archiveMissingLabel_ && IsWindow(archiveMissingLabel_)) {
            SetWindowPos(archiveMissingLabel_, nullptr, contentLeft, kContentTop, kContentWidth, kArchiveLabelHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }

        const int row1Y = kContentTop + kArchiveLabelHeight;
        const int row2Y = row1Y + kDownloadBtnHeight + kDownloadBtnGap;

        if (downloadMirror1Btn_ && IsWindow(downloadMirror1Btn_)) {
            SetWindowPos(downloadMirror1Btn_, nullptr, contentLeft, row1Y, kMirrorBtnWidth, kDownloadBtnHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (downloadMirror2Btn_ && IsWindow(downloadMirror2Btn_)) {
            SetWindowPos(downloadMirror2Btn_, nullptr, mirror2X, row1Y, kMirrorBtnWidth, kDownloadBtnHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (altDownloadCombo_ && IsWindow(altDownloadCombo_)) {
            SetWindowPos(altDownloadCombo_, nullptr, contentLeft, row2Y, kAltComboWidth, 200, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (altDownloadOpenBtn_ && IsWindow(altDownloadOpenBtn_)) {
            SetWindowPos(altDownloadOpenBtn_, nullptr, altOpenBtnX, row2Y, kAltOpenBtnWidth, kDownloadBtnHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    if (driveLabel_ && IsWindow(driveLabel_)) {
        SetWindowPos(driveLabel_, nullptr, contentLeft, layout.contentTop, kContentWidth, 24, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (driveCombo_ && IsWindow(driveCombo_)) {
        SetWindowPos(driveCombo_, nullptr, contentLeft, layout.driveComboY, kContentWidth, 300, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (showAllDrivesCheck_ && IsWindow(showAllDrivesCheck_)) {
        SetWindowPos(showAllDrivesCheck_, nullptr, contentLeft, layout.showAllDrivesY, kContentWidth, kCheckboxRowHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (formatCheck_ && IsWindow(formatCheck_)) {
        SetWindowPos(formatCheck_, nullptr, contentLeft, layout.formatY, kContentWidth, kCheckboxRowHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (ventoyActionCheck_ && IsWindow(ventoyActionCheck_)) {
        SetWindowPos(ventoyActionCheck_, nullptr, contentLeft, layout.ventoyActionY, kContentWidth, kCheckboxRowHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (advancedCheck_ && IsWindow(advancedCheck_)) {
        SetWindowPos(advancedCheck_, nullptr, contentLeft, layout.advancedY, 260, kCheckboxRowHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (pinVentoyCheck_ && IsWindow(pinVentoyCheck_)) {
        SetWindowPos(pinVentoyCheck_, nullptr, contentLeft + 20, layout.pinVentoyY, 260, kCheckboxRowHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (ventoyVersionCombo_ && IsWindow(ventoyVersionCombo_)) {
        SetWindowPos(ventoyVersionCombo_, nullptr, contentLeft + 280, layout.pinVentoyY - 2, 240, 300,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (ventoySecureBootCheck_ && IsWindow(ventoySecureBootCheck_)) {
        SetWindowPos(ventoySecureBootCheck_, nullptr, contentLeft + 20, layout.ventoySecureBootY, kContentWidth - 20,
                     kCheckboxRowHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (ventoyGptCheck_ && IsWindow(ventoyGptCheck_)) {
        SetWindowPos(ventoyGptCheck_, nullptr, contentLeft + 20, layout.gptY, kContentWidth - 20, kCheckboxRowHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (installBtn_ && IsWindow(installBtn_)) {
        SetWindowPos(installBtn_, nullptr, contentLeft, layout.installY, kInstallBtnWidth, kInstallBtnHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        EnableWindow(installBtn_, !archiveMissing_);
    }
    if (verifyFilesBtn_ && IsWindow(verifyFilesBtn_)) {
        SetWindowPos(verifyFilesBtn_, nullptr, verifyBtnX, layout.installY, kVerifyBtnWidth, kInstallBtnHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (progressBar_ && IsWindow(progressBar_)) {
        SetWindowPos(progressBar_, nullptr, contentLeft, layout.progressY, progressBarWidth, kProgressHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (openLogBtn_ && IsWindow(openLogBtn_)) {
        SetWindowPos(openLogBtn_, nullptr, openLogBtnX, layout.openLogY, kOpenLogBtnWidth, kOpenLogBtnHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (statusBar_ && IsWindow(statusBar_)) {
        SetWindowPos(statusBar_, nullptr, contentLeft, layout.statusBarY, kContentWidth, kStatusBarHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (betaNoticeLabel_ && IsWindow(betaNoticeLabel_)) {
        SetWindowPos(betaNoticeLabel_, nullptr, contentLeft, layout.betaNoticeY, kContentWidth, layout.betaNoticeHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (manualInstallBtn_ && IsWindow(manualInstallBtn_)) {
        SetWindowPos(manualInstallBtn_, nullptr, contentLeft, layout.manualInstallY, kContentWidth, kManualInstallBtnHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (creditsBtn_ && IsWindow(creditsBtn_)) {
        const auto footer = ComputeFooterButtonLayout(contentLeft);
        SetWindowPos(creditsBtn_, nullptr, footer.creditsX, layout.creditsBtnY, footer.sideBtnWidth, kCreditsBtnHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (discordFooterBtn_ && IsWindow(discordFooterBtn_)) {
        const auto footer = ComputeFooterButtonLayout(contentLeft);
        SetWindowPos(discordFooterBtn_, nullptr, footer.discordX, layout.creditsBtnY, footer.discordBtnWidth,
                     kCreditsBtnHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (feedbackBtn_ && IsWindow(feedbackBtn_)) {
        const auto footer = ComputeFooterButtonLayout(contentLeft);
        SetWindowPos(feedbackBtn_, nullptr, footer.feedbackX, layout.creditsBtnY, footer.sideBtnWidth, kCreditsBtnHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    int requiredClientHeight = layout.requiredClientHeight;
    HWND bottomFooterBtn = feedbackBtn_;
    if (!bottomFooterBtn || !IsWindow(bottomFooterBtn)) {
        bottomFooterBtn = discordFooterBtn_;
    }
    if (!bottomFooterBtn || !IsWindow(bottomFooterBtn)) {
        bottomFooterBtn = creditsBtn_;
    }
    if (bottomFooterBtn && IsWindow(bottomFooterBtn)) {
        RECT btnRect{};
        GetWindowRect(bottomFooterBtn, &btnRect);
        POINT bottomRight{btnRect.right, btnRect.bottom};
        ScreenToClient(hwnd_, &bottomRight);
        requiredClientHeight = std::max(requiredClientHeight, static_cast<int>(bottomRight.y) + kBottomChrome);
    } else if (manualInstallBtn_ && IsWindow(manualInstallBtn_)) {
        RECT btnRect{};
        GetWindowRect(manualInstallBtn_, &btnRect);
        POINT bottomRight{btnRect.right, btnRect.bottom};
        ScreenToClient(hwnd_, &bottomRight);
        requiredClientHeight = std::max(requiredClientHeight, static_cast<int>(bottomRight.y) + kBottomChrome);
    }

    const int targetClientWidth = kContentWidth + 2 * kMargin;
    const int targetOuterW = OuterWindowWidthForClient(hwnd_, targetClientWidth);
    const int targetOuterH = OuterWindowHeightForClient(hwnd_, targetClientWidth, requiredClientHeight);

    RECT wr{};
    GetWindowRect(hwnd_, &wr);
    if ((wr.right - wr.left) != targetOuterW || (wr.bottom - wr.top) != targetOuterH) {
        SetWindowPos(hwnd_, nullptr, 0, 0, targetOuterW, targetOuterH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hwnd_, nullptr, TRUE);
    }

    LayoutHeader();
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
        case WM_DEVICECHANGE:
            if (self->HandleDeviceChange(wp, lp)) {
                return 0;
            }
            break;
        case WM_MEDICAT_PROGRESS: {
            auto* payload = reinterpret_cast<ProgressPayload*>(lp);
            if (payload) {
                if (payload->downloadUpdate) {
                    self->SetDownloadProgress(payload->percent, payload->statusText, payload->file);
                } else if (payload->statusOnly) {
                    self->SetStatusBar(payload->statusText);
                } else if (payload->openFileLog) {
                    self->OpenFileLogWindow();
                } else if (payload->extractUpdate) {
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
            } else if (wp == kArchiveCheckTimerId) {
                self->UpdateArchivePanel();
            } else if (wp == kDriveRefreshTimerId) {
                KillTimer(hwnd, kDriveRefreshTimerId);
                self->OnDebouncedDriveChange();
            } else if (wp == kUpdateCheckTimerId) {
                if (self->onUpdateCheck_) {
                    self->onUpdateCheck_();
                }
                SetTimer(hwnd, kUpdateCheckTimerId, kUpdateCheckIntervalMs, nullptr);
            }
            return 0;
        case WM_SIZE:
            self->LayoutHeader();
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            const HDC hdc = BeginPaint(hwnd, &ps);
            RECT clientRect{};
            GetClientRect(hwnd, &clientRect);
            const int contentLeft = ContentLeft(clientRect.right - clientRect.left);
            RECT logoRect{contentLeft, kHeaderLogoY, contentLeft + kLogoMaxSize,
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
            if (ctl == self->versionLabel_ || ctl == self->statusBar_ || ctl == self->betaNoticeLabel_) {
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
        case WM_MEDICAT_REEXTRACT_PROMPT: {
            auto* payload = reinterpret_cast<ReExtractPromptPayload*>(lp);
            if (payload) {
                self->OpenReExtractPrompt(payload);
            }
            return 0;
        }
        case WM_MEDICAT_CONFIRM_PROMPT: {
            auto* payload = reinterpret_cast<ConfirmPromptPayload*>(lp);
            if (payload && payload->state) {
                const bool result =
                    self->ShowConfirmDialog(payload->message, payload->title, payload->kind);
                payload->state->result = result;
                payload->state->completed = true;
                if (payload->state->doneEvent) {
                    SetEvent(payload->state->doneEvent);
                }
            }
            delete payload;
            return 0;
        }
        case WM_MEDICAT_DRIVE_LIST: {
            auto* payload = reinterpret_cast<DriveListPayload*>(lp);
            if (payload) {
                self->ApplyDriveList(payload);
            }
            return 0;
        }
        case WM_MEDICAT_VENTOY_STATUS: {
            auto* payload = reinterpret_cast<VentoyStatusPayload*>(lp);
            if (payload) {
                self->ApplyVentoyStatus(payload);
            }
            return 0;
        }
        case WM_MEDICAT_UPDATE_RESULT: {
            auto* payload = reinterpret_cast<UpdateResultPayload*>(lp);
            if (payload) {
                if (payload->info.updateAvailable) {
                    self->ShowUpdatePrompt(payload->info);
                }
                delete payload;
            }
            return 0;
        }
        case WM_MEDICAT_FAILURE_DIAG: {
            auto* payload = reinterpret_cast<FailureDiagPayload*>(lp);
            if (payload) {
                self->SetFailureDiagCode(payload->uploadSucceeded, payload->keyword);
                delete payload;
            }
            return 0;
        }
        case WM_DESTROY:
            KillTimer(hwnd, kArchiveCheckTimerId);
            KillTimer(hwnd, kDriveRefreshTimerId);
            KillTimer(hwnd, kUpdateCheckTimerId);
            if (self->fileLogWindow_ && IsWindow(self->fileLogWindow_)) {
                DestroyWindow(self->fileLogWindow_);
            }
            if (self->reExtractWindow_ && IsWindow(self->reExtractWindow_)) {
                DestroyWindow(self->reExtractWindow_);
            }
            if (self->messageDialog_ && IsWindow(self->messageDialog_)) {
                DestroyWindow(self->messageDialog_);
            }
            if (self->creditsWindow_ && IsWindow(self->creditsWindow_)) {
                DestroyWindow(self->creditsWindow_);
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
    discordFooterIcon_ = theme::LoadEmbeddedIcon(instance_, IDI_DISCORD_ICON, kDiscordFooterIconSize);
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

    archiveMissingLabel_ = CreateWindowW(
        L"STATIC", i18n::Tr(L"ui.archive_missing", kMediCatArchiveFileName).c_str(),
        WS_CHILD | SS_LEFT | SS_LEFTNOWORDWRAP,
        kMargin, kContentTop, kContentWidth, kArchiveLabelHeight, hwnd, nullptr, instance_, nullptr);

    const auto createDownloadBtn = [&](const int id, const wchar_t* label, const int width) {
        return CreateWindowW(
            L"BUTTON", label, WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            0, 0, width, kDownloadBtnHeight, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    };

    downloadMirror1Btn_ = createDownloadBtn(kDownloadMirror1BtnId, kDownloadMirror1Name, kMirrorBtnWidth);
    downloadMirror2Btn_ = createDownloadBtn(kDownloadMirror2BtnId, kDownloadMirror2Name, kMirrorBtnWidth);

    altDownloadCombo_ = CreateWindowW(
        WC_COMBOBOXW, nullptr,
        CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL,
        0, 0, kAltComboWidth, 200, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAltDownloadComboId)), instance_, nullptr);
    SendMessageW(altDownloadCombo_, CB_SETDROPPEDWIDTH, kAltComboWidth + 24, 0);
    SendMessageW(altDownloadCombo_, CB_SETMINVISIBLE, 5, 0);

    altDownloadOpenBtn_ =
        createDownloadBtn(kAltDownloadOpenBtnId, i18n::Tr(L"ui.alt_download_open").c_str(), kAltOpenBtnWidth);

    const std::wstring versionText = InstallerVersionLabel();
    const MainContentLayout initialLayout = ComputeMainContentLayout(false, false);
    driveLabel_ = CreateWindowW(
        L"STATIC", i18n::Tr(L"ui.drive_label").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_LEFTNOWORDWRAP,
        kMargin, initialLayout.contentTop, kContentWidth, 24, hwnd, nullptr, instance_, nullptr);
    driveCombo_ = CreateWindowW(
        WC_COMBOBOXW, nullptr,
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        kMargin, initialLayout.driveComboY, kContentWidth, 300, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDriveComboId)), instance_, nullptr);

    showAllDrivesCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.show_all_drives").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, initialLayout.showAllDrivesY, kContentWidth, kCheckboxRowHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kShowAllDrivesCheckId)), instance_, nullptr);

    formatCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.format_checkbox").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, initialLayout.formatY, kContentWidth, kCheckboxRowHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kFormatCheckId)), instance_, nullptr);
    SendMessageW(formatCheck_, BM_SETCHECK, BST_CHECKED, 0);

    ventoyActionCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.install_ventoy_checkbox").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, initialLayout.ventoyActionY, kContentWidth, kCheckboxRowHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVentoyActionCheckId)), instance_, nullptr);

    advancedCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.advanced_options").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin, initialLayout.advancedY, 260, kCheckboxRowHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAdvancedCheckId)), instance_, nullptr);

    pinVentoyCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.pin_ventoy_version").c_str(),
        WS_CHILD | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin + 20, initialLayout.pinVentoyY, 260, kCheckboxRowHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPinVentoyCheckId)), instance_, nullptr);

    ventoyVersionCombo_ = CreateWindowW(
        WC_COMBOBOXW, nullptr,
        CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL,
        270, initialLayout.pinVentoyY - 2, 270, 300, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVentoyVersionEditId)), instance_, nullptr);

    ventoySecureBootCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.ventoy_secure_boot").c_str(),
        WS_CHILD | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin + 20, initialLayout.ventoySecureBootY, kContentWidth - 20, kCheckboxRowHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVentoySecureBootCheckId)), instance_, nullptr);
    SendMessageW(ventoySecureBootCheck_, BM_SETCHECK, BST_CHECKED, 0);

    ventoyGptCheck_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.ventoy_gpt_partition").c_str(),
        WS_CHILD | BS_AUTOCHECKBOX | BS_MULTILINE,
        kMargin + 20, initialLayout.gptY, kContentWidth - 20, kCheckboxRowHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVentoyGptCheckId)), instance_, nullptr);

    installBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.install_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        kMargin, initialLayout.installY, kInstallBtnWidth, kInstallBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kInstallBtnId)), instance_, nullptr);

    verifyFilesBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.check_files_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        kMargin + kInstallBtnWidth + kActionBtnGap, initialLayout.installY, kVerifyBtnWidth, kInstallBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVerifyFilesBtnId)), instance_, nullptr);

    progressBar_ = CreateWindowW(
        PROGRESS_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        kMargin, initialLayout.progressY, kProgressBarWidth, kProgressHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kProgressId)), instance_, nullptr);
    SendMessageW(progressBar_, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SetPropW(progressBar_, L"MedicatGui", this);
    const auto originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        progressBar_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ProgressBarProc)));
    SetWindowLongPtrW(progressBar_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(originalProc));

    openLogBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.open_file_log_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        kMargin + kProgressBarWidth + kActionBtnGap, initialLayout.openLogY, kOpenLogBtnWidth, kOpenLogBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOpenLogBtnId)), instance_, nullptr);

    // Single-line status bar: any code may update via SetStatusBar (UI thread) or App::PostStatusBar (workers).
    statusBar_ = CreateWindowW(
        L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS,
        kMargin, initialLayout.statusBarY, kContentWidth, kStatusBarHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusBarId)), instance_, nullptr);

    betaNoticeLabel_ = CreateWindowW(
        L"STATIC", i18n::Tr(L"ui.beta_telemetry_notice").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        kMargin, initialLayout.betaNoticeY, kContentWidth, kBetaNoticeMinHeight, hwnd, nullptr, instance_, nullptr);

    manualInstallBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.manual_install_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        kMargin, initialLayout.manualInstallY, kContentWidth, kManualInstallBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kManualInstallBtnId)), instance_, nullptr);

    const auto footer = ComputeFooterButtonLayout(kMargin);

    creditsBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.credits_licenses_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        footer.creditsX, initialLayout.creditsBtnY, footer.sideBtnWidth, kCreditsBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCreditsBtnId)), instance_, nullptr);

    discordFooterBtn_ = CreateWindowW(
        L"BUTTON", L"Discord", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        footer.discordX, initialLayout.creditsBtnY, footer.discordBtnWidth, kCreditsBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDiscordFooterBtnId)), instance_, nullptr);

    feedbackBtn_ = CreateWindowW(
        L"BUTTON", i18n::Tr(L"ui.feedback_button").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        footer.feedbackX, initialLayout.creditsBtnY, footer.sideBtnWidth, kCreditsBtnHeight, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kFeedbackBtnId)), instance_, nullptr);

    versionLabel_ = CreateWindowW(
        L"STATIC", versionText.c_str(),
        WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX,
        kLanguageComboX, kVersionLabelY, kVersionLabelWidth, kVersionLabelHeight, hwnd, nullptr, instance_, nullptr);

    for (HWND child :
         {languageCombo_, titleLabel_, versionLabel_, archiveMissingLabel_, downloadMirror1Btn_, downloadMirror2Btn_,
          altDownloadCombo_, altDownloadOpenBtn_, driveLabel_, driveCombo_, showAllDrivesCheck_, formatCheck_, ventoyActionCheck_, advancedCheck_,
          pinVentoyCheck_, ventoySecureBootCheck_, ventoyGptCheck_, ventoyVersionCombo_, installBtn_, verifyFilesBtn_,
          openLogBtn_, manualInstallBtn_, creditsBtn_, discordFooterBtn_, feedbackBtn_, progressBar_, statusBar_,
          betaNoticeLabel_}) {
        if (child && IsWindow(child)) {
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        }
    }
    SendMessageW(titleLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont), TRUE);
    SendMessageW(versionLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(subtitleFont), TRUE);
    SendMessageW(betaNoticeLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(subtitleFont), TRUE);
    SubclassWrappedStatic(betaNoticeLabel_, true);

    theme::EnableDarkMode(hwnd);
    theme::EnableDarkModeRecursive(hwnd);

    SubclassGlowButton(installBtn_, true);
    SubclassGlowButton(verifyFilesBtn_, false);
    SubclassGlowButton(openLogBtn_, false);
    SubclassGlowButton(manualInstallBtn_, false);
    SubclassGlowButton(creditsBtn_, false);
    SubclassGlowIconButton(discordFooterBtn_, discordFooterIcon_, true);
    SubclassGlowButton(feedbackBtn_, false);
    SubclassGlowButton(downloadMirror1Btn_, true);
    SubclassGlowButton(downloadMirror2Btn_, true);
    SubclassGlowButton(altDownloadOpenBtn_, false);

    for (const HWND checkbox : {formatCheck_, ventoyActionCheck_, showAllDrivesCheck_, advancedCheck_, pinVentoyCheck_,
                                ventoySecureBootCheck_, ventoyGptCheck_}) {
        SubclassFlatCheckbox(checkbox);
        InvalidateRect(checkbox, nullptr, TRUE);
    }

    PopulateAlternativeDownloadCombo();
    UpdateAdvancedControls();
    LayoutHeader();
    UpdateArchivePanel();
    LayoutMainContent();
    SetTimer(hwnd_, kArchiveCheckTimerId, kArchiveCheckIntervalMs, nullptr);
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
    if (id == kFormatCheckId || id == kVentoyActionCheckId) {
        EnforceForcedDriveCheckboxes();
        return;
    }
    if (id == kDriveComboId && HIWORD(wp) == CBN_SELCHANGE) {
        RefreshDriveVentoyStatus();
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
        return;
    }
    if (id == kManualInstallBtnId) {
        OpenBrowserUrl(kManualInstallDocUrl);
        return;
    }
    if (id == kCreditsBtnId) {
        OpenCreditsWindow();
        return;
    }
    if (id == kDiscordFooterBtnId) {
        OpenBrowserUrl(kDiscordSupportUrl);
        return;
    }
    if (id == kFeedbackBtnId) {
        OpenBrowserUrl(kBetaFeedbackUrl);
        return;
    }
    if (id == kDownloadMirror1BtnId) {
        StartMirrorDownload(kDownloadMirror1Url, kDownloadMirror1Name);
        return;
    }
    if (id == kDownloadMirror2BtnId) {
        StartMirrorDownload(kDownloadMirror2Url, kDownloadMirror2Name);
        return;
    }
    if (id == kAltDownloadOpenBtnId) {
        OpenSelectedAlternativeDownload();
    }
}

void Gui::SetInitialLanguage(const std::wstring& languageCode) {
    ApplyLanguageSelection(languageCode);
}

void Gui::ApplyLanguageSelection(const std::wstring& languageCode) {
    i18n::Load(languageCode);
    if (languageCombo_ && IsWindow(languageCombo_)) {
        const int selected = LanguageIndexForCode(languageCode);
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

    if (hwnd_ && IsWindow(hwnd_)) {
        SetWindowTextW(hwnd_, i18n::Tr(L"ui.form_title", InstallerVersionWide()).c_str());
    }

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
    if (ventoyActionCheck_ && IsWindow(ventoyActionCheck_)) {
        const wchar_t* labelKey =
            ventoyOnDrive_ ? L"ui.update_ventoy_checkbox" : L"ui.install_ventoy_checkbox";
        SetWindowTextW(ventoyActionCheck_, i18n::Tr(labelKey).c_str());
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
    if (manualInstallBtn_ && IsWindow(manualInstallBtn_)) {
        SetWindowTextW(manualInstallBtn_, i18n::Tr(L"ui.manual_install_button").c_str());
    }
    if (creditsBtn_ && IsWindow(creditsBtn_)) {
        SetWindowTextW(creditsBtn_, i18n::Tr(L"ui.credits_licenses_button").c_str());
    }
    if (feedbackBtn_ && IsWindow(feedbackBtn_)) {
        SetWindowTextW(feedbackBtn_, i18n::Tr(L"ui.feedback_button").c_str());
    }
    if (betaNoticeLabel_ && IsWindow(betaNoticeLabel_)) {
        SetWindowTextW(betaNoticeLabel_, i18n::Tr(L"ui.beta_telemetry_notice").c_str());
    }
    RefreshMessageDialogText();
    RefreshCreditsWindowText();
    if (archiveMissingLabel_ && IsWindow(archiveMissingLabel_)) {
        RefreshArchivePanelLabel();
    }
    if (downloadMirror1Btn_ && IsWindow(downloadMirror1Btn_)) {
        SetWindowTextW(downloadMirror1Btn_, kDownloadMirror1Name);
    }
    if (downloadMirror2Btn_ && IsWindow(downloadMirror2Btn_)) {
        SetWindowTextW(downloadMirror2Btn_, kDownloadMirror2Name);
    }
    if (altDownloadOpenBtn_ && IsWindow(altDownloadOpenBtn_)) {
        SetWindowTextW(altDownloadOpenBtn_, i18n::Tr(L"ui.alt_download_open").c_str());
    }
    PopulateAlternativeDownloadCombo();
    if (fileLogWindow_ && IsWindow(fileLogWindow_)) {
        SetWindowTextW(fileLogWindow_, i18n::Tr(L"ui.file_log_title").c_str());
    }
    if (versionLabel_ && IsWindow(versionLabel_)) {
        SetWindowTextW(versionLabel_, InstallerVersionLabel().c_str());
    }

    for (HWND child : {titleLabel_, archiveMissingLabel_, downloadMirror1Btn_, downloadMirror2Btn_, altDownloadCombo_,
                       altDownloadOpenBtn_, driveLabel_, driveCombo_, showAllDrivesCheck_, formatCheck_,
                       ventoyActionCheck_, advancedCheck_, pinVentoyCheck_, ventoySecureBootCheck_, ventoyGptCheck_,
                       installBtn_, verifyFilesBtn_, openLogBtn_, manualInstallBtn_, creditsBtn_, progressBar_, statusBar_,
                       betaNoticeLabel_, languageCombo_, versionLabel_}) {
        refreshControl(child);
    }
    LayoutMainContent();
}

void Gui::RefreshDrives(const bool fromDeviceChange) {
    RequestDriveRefresh(fromDeviceChange);
}

void Gui::RequestDriveRefresh(const bool fromDeviceChange) {
    if (!driveCombo_ || !IsWindow(driveCombo_)) {
        return;
    }

    if (driveRefreshInFlight_.load(std::memory_order_acquire)) {
        driveRefreshCoalesce_.store(true, std::memory_order_release);
        if (fromDeviceChange) {
            driveRefreshCoalesceFromDevice_ = true;
        }
        return;
    }

    std::vector<std::wstring> lettersBefore;
    std::wstring selectedBefore;
    if (fromDeviceChange) {
        lettersBefore = CollectComboDriveLetters(driveCombo_);
        selectedBefore = SelectedDrive();
    }

    std::wstring previous;
    const int previousIdx = static_cast<int>(SendMessageW(driveCombo_, CB_GETCURSEL, 0, 0));
    if (previousIdx >= 0) {
        const auto* letter =
            reinterpret_cast<std::wstring*>(SendMessageW(driveCombo_, CB_GETITEMDATA, previousIdx, 0));
        if (letter) {
            previous = *letter;
        }
    }

    const bool showAll = ShowAllDrivesChecked();
    const uint64_t generation = ++driveListGeneration_;
    const HWND hwnd = hwnd_;

    SetStatusBar(i18n::Tr(L"status.scanning_drives"));
    driveRefreshInFlight_.store(true, std::memory_order_release);

    std::thread([hwnd, showAll, fromDeviceChange, lettersBefore = std::move(lettersBefore),
                 selectedBefore = std::move(selectedBefore), previous = std::move(previous), generation]() {
        auto* payload = new DriveListPayload{};
        payload->drives = ListTargetDrives(showAll);
        payload->lettersBefore = std::move(lettersBefore);
        payload->selectedBefore = std::move(selectedBefore);
        payload->previous = std::move(previous);
        payload->fromDeviceChange = fromDeviceChange;
        payload->generation = generation;
        PostMessageW(hwnd, WM_MEDICAT_DRIVE_LIST, 0, reinterpret_cast<LPARAM>(payload));
    }).detach();
}

void Gui::ApplyDriveList(DriveListPayload* payload) {
    if (!payload || !driveCombo_ || !IsWindow(driveCombo_)) {
        delete payload;
        driveRefreshInFlight_.store(false, std::memory_order_release);
        return;
    }

    std::unique_ptr<DriveListPayload> guard(payload);
    if (payload->generation != driveListGeneration_.load(std::memory_order_acquire)) {
        driveRefreshInFlight_.store(false, std::memory_order_release);
        if (driveRefreshCoalesce_.exchange(false, std::memory_order_acq_rel)) {
            const bool fromDevice = driveRefreshCoalesceFromDevice_;
            driveRefreshCoalesceFromDevice_ = false;
            RequestDriveRefresh(fromDevice);
        } else {
            RefreshDriveVentoyStatus();
        }
        return;
    }

    const int count = static_cast<int>(SendMessageW(driveCombo_, CB_GETCOUNT, 0, 0));
    for (int i = 0; i < count; ++i) {
        const auto* letter = reinterpret_cast<std::wstring*>(SendMessageW(driveCombo_, CB_GETITEMDATA, i, 0));
        delete letter;
    }

    SendMessageW(driveCombo_, CB_RESETCONTENT, 0, 0);
    const auto& drives = payload->drives;
    int restoreIdx = -1;
    for (size_t i = 0; i < drives.size(); ++i) {
        const auto& d = drives[i];
        SendMessageW(driveCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(d.display.c_str()));
        const int idx = static_cast<int>(SendMessageW(driveCombo_, CB_GETCOUNT, 0, 0)) - 1;
        SendMessageW(driveCombo_, CB_SETITEMDATA, idx, reinterpret_cast<LPARAM>(new std::wstring(d.letter)));
        if (!payload->previous.empty() && d.letter == payload->previous) {
            restoreIdx = idx;
        }
    }

    const int def = restoreIdx >= 0 ? restoreIdx : DefaultDriveIndex(drives);
    if (def >= 0) {
        SendMessageW(driveCombo_, CB_SETCURSEL, def, 0);
    }
    LogDriveListSelection(drives, def, payload->previous, restoreIdx);

    const bool notified = payload->fromDeviceChange &&
                          ApplyDriveChangeNotifications(payload->lettersBefore, drives, payload->selectedBefore);
    RefreshDriveVentoyStatus(!notified);

    driveRefreshInFlight_.store(false, std::memory_order_release);
    if (driveRefreshCoalesce_.exchange(false, std::memory_order_acq_rel)) {
        const bool fromDevice = driveRefreshCoalesceFromDevice_;
        driveRefreshCoalesceFromDevice_ = false;
        RequestDriveRefresh(fromDevice);
    }
}

bool Gui::HandleDeviceChange(const WPARAM wp, const LPARAM lp) {
    if (wp != DBT_DEVICEARRIVAL && wp != DBT_DEVICEREMOVECOMPLETE) {
        return false;
    }

    const auto* hdr = reinterpret_cast<const DEV_BROADCAST_HDR*>(lp);
    if (!hdr || hdr->dbch_devicetype != DBT_DEVTYP_VOLUME) {
        return false;
    }

    ScheduleDriveChangeRefresh();
    return true;
}

void Gui::ScheduleDriveChangeRefresh() {
    if (!hwnd_ || !IsWindow(hwnd_)) {
        return;
    }
    SetTimer(hwnd_, kDriveRefreshTimerId, kDriveDebounceMs, nullptr);
}

void Gui::OnDebouncedDriveChange() {
    if (busyProgressMode_ != BusyProgressMode::None) {
        const std::wstring selected = SelectedDrive();
        if (!selected.empty() && !IsDriveLetterPresent(selected)) {
            pendingDriveRefresh_ = true;
            SetStatusBar(i18n::Tr(L"status.selected_drive_removed", selected));
            if (onLog_) {
                onLog_(L"Selected drive removed during operation: " + selected);
            }
        }
        return;
    }

    pendingDriveRefresh_ = false;
    RefreshDrives(true);
}

bool Gui::ApplyDriveChangeNotifications(const std::vector<std::wstring>& lettersBefore,
                                        const std::vector<DriveInfo>& drives,
                                        const std::wstring& selectedBefore) {
    const auto containsLetter = [&](const std::wstring& letter) {
        return std::any_of(drives.begin(), drives.end(),
                           [&](const DriveInfo& drive) { return drive.letter == letter; });
    };

    if (!selectedBefore.empty() && !containsLetter(selectedBefore)) {
        SetStatusBar(i18n::Tr(L"status.selected_drive_removed", selectedBefore));
        if (onLog_) {
            onLog_(L"Selected drive removed: " + selectedBefore);
        }
        return true;
    }

    std::vector<const DriveInfo*> arrived;
    for (const auto& drive : drives) {
        const bool wasListed =
            std::any_of(lettersBefore.begin(), lettersBefore.end(),
                        [&](const std::wstring& letter) { return letter == drive.letter; });
        if (!wasListed) {
            arrived.push_back(&drive);
        }
    }

    if (arrived.empty()) {
        return false;
    }

    if (arrived.size() == 1) {
        const DriveInfo& drive = *arrived[0];
        for (size_t i = 0; i < drives.size(); ++i) {
            if (drives[i].letter == drive.letter) {
                SendMessageW(driveCombo_, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
                lastVentoyControlDrive_.clear();
                RefreshDriveVentoyControls();
                break;
            }
        }
        SetStatusBar(i18n::Tr(L"status.drive_arrived", drive.display));
        if (onLog_) {
            onLog_(L"Drive arrived: " + drive.display);
        }
        return true;
    }

    SetStatusBar(i18n::Tr(L"status.drives_arrived", std::to_wstring(arrived.size())));
    if (onLog_) {
        onLog_(L"Drive list updated: " + std::to_wstring(arrived.size()) + L" new drive(s)");
    }
    return true;
}

void Gui::RefreshDriveVentoyStatus(const bool updateStatusBar) {
    if (busyProgressMode_ != BusyProgressMode::None) {
        return;
    }

    const std::wstring drive = SelectedDrive();
    if (drive.empty()) {
        hasLastVentoyLog_ = false;
        ventoyOnDrive_ = false;

        if (drive != lastVentoyControlDrive_) {
            lastVentoyControlDrive_ = drive;
            RefreshDriveVentoyControls();
        }

        if (updateStatusBar) {
            SetStatusBar(i18n::Tr(L"status.status_ready"));
        }
        return;
    }

    const uint64_t generation = ++ventoyStatusGeneration_;
    const HWND hwnd = hwnd_;
    std::thread([hwnd, drive, updateStatusBar, generation]() {
        auto* payload = new VentoyStatusPayload{};
        payload->drive = drive;
        payload->detection = DetectVentoyOnDrive(drive);
        payload->updateStatusBar = updateStatusBar;
        payload->generation = generation;
        PostMessageW(hwnd, WM_MEDICAT_VENTOY_STATUS, 0, reinterpret_cast<LPARAM>(payload));
    }).detach();
}

void Gui::ApplyVentoyStatus(VentoyStatusPayload* payload) {
    if (!payload) {
        return;
    }

    std::unique_ptr<VentoyStatusPayload> guard(payload);
    if (payload->generation != ventoyStatusGeneration_.load(std::memory_order_acquire)) {
        return;
    }
    if (payload->drive != SelectedDrive()) {
        return;
    }

    LogVentoyDetection(payload->drive, payload->detection);
    ventoyOnDrive_ = payload->detection.installed;

    if (payload->drive != lastVentoyControlDrive_) {
        lastVentoyControlDrive_ = payload->drive;
        RefreshDriveVentoyControls();
    }

    if (payload->updateStatusBar) {
        if (ventoyOnDrive_) {
            SetStatusBar(i18n::Tr(L"status.ventoy_found", payload->drive));
        } else {
            SetStatusBar(i18n::Tr(L"status.ventoy_not_on_drive", payload->drive));
        }
    }
}

void Gui::RefreshDriveVentoyControls() {
    const std::wstring drive = SelectedDrive();

    if (ventoyActionCheck_ && IsWindow(ventoyActionCheck_)) {
        if (drive.empty()) {
            SetWindowTextW(ventoyActionCheck_, i18n::Tr(L"ui.install_ventoy_checkbox").c_str());
            SendMessageW(ventoyActionCheck_, BM_SETCHECK, BST_UNCHECKED, 0);
        } else if (ventoyOnDrive_) {
            SetWindowTextW(ventoyActionCheck_, i18n::Tr(L"ui.update_ventoy_checkbox").c_str());
            SendMessageW(ventoyActionCheck_, BM_SETCHECK, BST_UNCHECKED, 0);
        } else {
            SetWindowTextW(ventoyActionCheck_, i18n::Tr(L"ui.install_ventoy_checkbox").c_str());
            SendMessageW(ventoyActionCheck_, BM_SETCHECK, BST_CHECKED, 0);
        }
    }

    if (formatCheck_ && IsWindow(formatCheck_)) {
        if (drive.empty() || ventoyOnDrive_) {
            SendMessageW(formatCheck_, BM_SETCHECK, BST_UNCHECKED, 0);
        } else {
            SendMessageW(formatCheck_, BM_SETCHECK, BST_CHECKED, 0);
        }
    }
}

void Gui::EnforceForcedDriveCheckboxes() {
    const std::wstring drive = SelectedDrive();
    if (!RequiresForcedVentoyInstall(ventoyOnDrive_, drive)) {
        return;
    }

    if (ventoyActionCheck_ && IsWindow(ventoyActionCheck_)) {
        SendMessageW(ventoyActionCheck_, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (formatCheck_ && IsWindow(formatCheck_)) {
        SendMessageW(formatCheck_, BM_SETCHECK, BST_CHECKED, 0);
    }
}

}  // namespace medicat
