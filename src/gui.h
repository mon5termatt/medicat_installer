#pragma once

#include "drives.h"
#include "update.h"
#include "ventoy.h"
#include "verify.h"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace medicat {

constexpr UINT WM_MEDICAT_PROGRESS = WM_APP + 1;
constexpr UINT WM_MEDICAT_DONE = WM_APP + 2;
constexpr UINT WM_MEDICAT_VENTOY_VERSIONS = WM_APP + 3;
constexpr UINT WM_MEDICAT_REEXTRACT_PROMPT = WM_APP + 4;
constexpr UINT WM_MEDICAT_DRIVE_LIST = WM_APP + 5;
constexpr UINT WM_MEDICAT_VENTOY_STATUS = WM_APP + 6;
constexpr UINT WM_MEDICAT_UPDATE_RESULT = WM_APP + 7;
constexpr UINT WM_MEDICAT_FAILURE_DIAG = WM_APP + 8;
constexpr UINT WM_MEDICAT_CONFIRM_PROMPT = WM_APP + 9;

enum class MessageDialogKind { Info, Warning, Error };

enum class MessageDialogFooter { Ok, OkWithDiag, YesNo };

enum class BusyProgressMode { FileLog, Verify, Download, None };

struct ConfirmPromptState {
    HANDLE doneEvent = nullptr;
    std::atomic<bool> result{false};
    std::atomic<bool> completed{false};
};

struct ConfirmPromptPayload {
    std::wstring message;
    std::wstring title;
    MessageDialogKind kind = MessageDialogKind::Warning;
    std::shared_ptr<ConfirmPromptState> state;
};

struct ReExtractPromptState {
    HANDLE doneEvent = nullptr;
    std::atomic<bool> wantReExtract{false};
    std::atomic<bool> completed{false};
};

struct ReExtractPromptPayload {
    std::wstring message;
    std::wstring title;
    size_t failedFiles = 0;
    std::vector<std::wstring> failures;
    std::shared_ptr<ReExtractPromptState> state;
};

struct ProgressPayload {
    int percent = 0;
    bool clearLog = false;
    bool resetLog = false;
    bool extractUpdate = false;
    bool downloadUpdate = false;
    // When true, only statusText is applied to the main-window status bar (see Gui::SetStatusBar).
    bool statusOnly = false;
    bool openFileLog = false;
    bool setBusyMode = false;
    BusyProgressMode busyProgressMode = BusyProgressMode::FileLog;
    std::wstring file;
    std::wstring statusText;
};

struct DonePayload {
    bool success = false;
    bool refreshArchivePanel = false;
    std::wstring message;
    std::wstring title;
};

struct DriveListPayload {
    std::vector<DriveInfo> drives;
    std::vector<std::wstring> lettersBefore;
    std::wstring selectedBefore;
    std::wstring previous;
    bool fromDeviceChange = false;
    uint64_t generation = 0;
};

struct VentoyStatusPayload {
    std::wstring drive;
    VentoyDetectionResult detection;
    MedicatPresenceResult medicatPresence;
    bool updateStatusBar = true;
    uint64_t generation = 0;
};

struct UpdateResultPayload {
    InstallerUpdateInfo info;
};

struct FailureDiagPayload {
    bool uploadSucceeded = false;
    std::wstring keyword;
};

class Gui {
public:
    using InstallHandler = std::function<void()>;

    bool Create(HINSTANCE instance);
    int Run();
    void SetInstallHandler(InstallHandler handler);
    void SetVerifyHandler(InstallHandler handler);
    void SetLogHandler(std::function<void(const std::wstring&)> handler);
    void SetUpdateCheckHandler(std::function<void()> handler);
    void SetApplyInstallerUpdateHandler(std::function<void(const InstallerUpdateInfo&)> handler);
    void ScheduleUpdateCheck();
    void SetBusy(bool busy, BusyProgressMode progressMode = BusyProgressMode::FileLog);
    void SetProgress(int percent, bool clearLog = false);
    void SetDownloadProgress(int percent, const std::wstring& barText, const std::wstring& labelText);
    // Single-line status bar directly below the progress bar. Any UI or worker code may
    // update it (workers should post WM_MEDICAT_PROGRESS with statusOnly, or use App::PostStatusBar).
    void SetStatusBar(const std::wstring& text);
    void ClearStatusBar();
    void NotifyExtractProgress(int percent, const std::wstring& file = L"", bool resetLog = false);
    // Clears the optional detail file-log popup only; does not touch the status bar.
    void ClearFileLog();
    void OpenFileLogWindow();
    void OpenCreditsWindow();
    void OpenReExtractPrompt(ReExtractPromptPayload* payload);
    void FinishReExtractPrompt(bool wantReExtract);
    void OpenFailureDialog(const std::wstring& message, const std::wstring& title);
    void SetFailureDiagCode(bool uploadSucceeded, const std::wstring& keyword);
    void ResetFailureDiagCode();
    void ShowMessageDialog(const std::wstring& message, const std::wstring& title, MessageDialogKind kind);
    bool ShowConfirmDialog(const std::wstring& message, const std::wstring& title,
                           MessageDialogKind kind = MessageDialogKind::Warning);
    bool ShowHelpGateDialog(int failureCount);
    void ShowDone(bool success, const std::wstring& message, const std::wstring& title = L"",
                  bool refreshArchivePanel = false);
    void UpdateArchivePanel();
    void SetInitialLanguage(const std::wstring& languageCode);
    std::wstring SelectedDrive() const;
    // User-browsed archive path (empty = use beside-exe / offline resolution).
    std::wstring SelectedArchivePath() const;
    bool FormatChecked() const;
    bool RunVentoyChecked() const;
    bool VentoyOnSelectedDrive() const;
    bool MedicatOnSelectedDrive() const;
    bool AdvancedChecked() const;
    bool PinVentoyVersionChecked() const;
    bool VentoySecureBootChecked() const;
    bool VentoyGptChecked() const;
    bool ShowAllDrivesChecked() const;
    std::wstring PinnedVentoyVersion() const;
    HWND Hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK ProgressBarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK FileLogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK CreditsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK ReExtractWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK MessageDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK HelpGateWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void OnCreate(HWND hwnd);
    void OnCommand(WPARAM wp, LPARAM lp = 0);
    void RefreshDrives(bool fromDeviceChange = false);
    void RequestDriveRefresh(bool fromDeviceChange = false);
    void ApplyDriveList(DriveListPayload* payload);
    void RefreshDriveVentoyStatus(bool updateStatusBar = true);
    void ApplyVentoyStatus(VentoyStatusPayload* payload);
    void RefreshDriveVentoyControls();
    void UpdateVerifyFilesButton();
    bool HandleDeviceChange(WPARAM wp, LPARAM lp);
    void ScheduleDriveChangeRefresh();
    void OnDebouncedDriveChange();
    bool ApplyDriveChangeNotifications(const std::vector<std::wstring>& lettersBefore,
                                       const std::vector<DriveInfo>& drives,
                                       const std::wstring& selectedBefore);
    void EnforceForcedDriveCheckboxes();
    void LogVentoyDetection(const std::wstring& drive, const VentoyDetectionResult& detection);
    void LogMedicatPresence(const std::wstring& drive, const MedicatPresenceResult& presence);
    void LogDriveListSelection(const std::vector<DriveInfo>& drives, int selectedIdx, const std::wstring& previous,
                               int restoreIdx);
    void FlushInstallUi();
    void BatchAppendDetailLog(const std::vector<std::wstring>& files, size_t startIndex);
    void SyncDetailLog();
    void ResizeFileLogWindow(HWND hwnd);
    std::wstring FormatLogLine(size_t index, const std::wstring& path) const;
    void UpdateAdvancedControls();
    void LayoutHeader();
    void LayoutMainContent();
    void RefreshCreditsWindowText();
    void LayoutCreditsWindow();
    void OpenMessageDialogInternal(const std::wstring& message, const std::wstring& title, MessageDialogKind kind,
                                   MessageDialogFooter footer, bool modal);
    void LayoutMessageDialog();
    void RefreshMessageDialogText();
    bool RunMessageDialogModalLoop();
    void LayoutHelpGateDialog();
    void RefreshHelpGateText();
    bool RunHelpGateModalLoop();
    bool CopyFailureDiagCodeToClipboard();
    std::wstring FailureDiagDisplayText() const;
    COLORREF MessageDialogAccentColor(MessageDialogKind kind) const;
    void LayoutReExtractDialog();
    void RefreshArchivePanelLabel();
    bool IsArchiveAvailable() const;
    std::wstring EffectiveArchivePath() const;
    void BrowseForArchive();
    void EnsureVentoyVersionsLoaded();
    void PopulateVentoyVersionCombo();
    void SetVentoyVersions(std::vector<std::wstring> versions);
    void ApplyLanguageSelection(const std::wstring& languageCode);
    void RefreshTranslatedUi();
    void StartMirrorDownload(const std::wstring& url, const std::wstring& mirrorName);
    void StartTorrentDownload();
    void StartArchiveDownload(const std::wstring& url, const std::wstring& sourceName, bool torrent);
    void SetDownloadControlsEnabled(bool enabled);
    void ShowUpdatePrompt(const InstallerUpdateInfo& info);
    bool LogoHitTest(HWND hwnd, int clientX, int clientY) const;
    void TryOpenDebugMenuFromLogoClick(HWND source);
    void OpenDebugMenu(int screenX, int screenY);
    void HandleDebugMenuCommand(int id);

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND logoStatic_ = nullptr;
    HWND titleLabel_ = nullptr;
    HWND versionLabel_ = nullptr;
    HWND driveLabel_ = nullptr;
    HWND languageCombo_ = nullptr;
    HWND driveCombo_ = nullptr;
    HWND showAllDrivesCheck_ = nullptr;
    HWND formatCheck_ = nullptr;
    HWND ventoyActionCheck_ = nullptr;
    HWND advancedCheck_ = nullptr;
    HWND pinVentoyCheck_ = nullptr;
    HWND ventoySecureBootCheck_ = nullptr;
    HWND ventoyGptCheck_ = nullptr;
    HWND ventoyVersionCombo_ = nullptr;
    HWND installBtn_ = nullptr;
    HWND verifyFilesBtn_ = nullptr;
    HWND openLogBtn_ = nullptr;
    HWND manualInstallBtn_ = nullptr;
    HWND creditsBtn_ = nullptr;
    HWND discordFooterBtn_ = nullptr;
    HWND feedbackBtn_ = nullptr;
    HWND progressBar_ = nullptr;
    HWND statusBar_ = nullptr;
    HWND betaNoticeLabel_ = nullptr;
    HWND archiveMissingLabel_ = nullptr;
    HWND downloadMirror1Btn_ = nullptr;
    HWND downloadMirror2Btn_ = nullptr;
    HWND downloadTorrentBtn_ = nullptr;
    HWND downloadMagnetBtn_ = nullptr;
    HWND downloadGoogleDriveBtn_ = nullptr;
    HWND downloadMegaBtn_ = nullptr;
    HWND browseArchiveBtn_ = nullptr;
    HWND fileLogWindow_ = nullptr;
    HWND fileLogList_ = nullptr;
    HWND creditsWindow_ = nullptr;
    HWND creditsIntro_ = nullptr;
    HWND creditsSevenZipBtn_ = nullptr;
    HWND creditsAria2Btn_ = nullptr;
    HWND creditsVentoyBtn_ = nullptr;
    HWND creditsCloseBtn_ = nullptr;
    HWND reExtractWindow_ = nullptr;
    HWND reExtractMessage_ = nullptr;
    HWND reExtractList_ = nullptr;
    HWND reExtractBtn_ = nullptr;
    HWND reExtractCloseBtn_ = nullptr;
    std::shared_ptr<ReExtractPromptState> activeReExtractPrompt_;
    HWND messageDialog_ = nullptr;
    HWND messageDialogBody_ = nullptr;
    HWND messageDialogDiagLabel_ = nullptr;
    HWND messageDialogDiagEdit_ = nullptr;
    HWND messageDialogCopyBtn_ = nullptr;
    HWND messageDialogDiscordBtn_ = nullptr;
    HWND messageDialogOkBtn_ = nullptr;
    HWND messageDialogYesBtn_ = nullptr;
    HWND messageDialogNoBtn_ = nullptr;
    HWND helpGateDialog_ = nullptr;
    HWND helpGateBody_ = nullptr;
    HWND helpGateDiscordBtn_ = nullptr;
    HWND helpGateAckCheckbox_ = nullptr;
    HWND helpGateContinueBtn_ = nullptr;
    bool helpGateModalActive_ = false;
    bool helpGateModalResult_ = false;
    int helpGateFailureCount_ = 0;
    int helpGateBodyHeight_ = 0;
    MessageDialogKind messageDialogKind_ = MessageDialogKind::Error;
    MessageDialogFooter messageDialogFooter_ = MessageDialogFooter::Ok;
    bool messageDialogModal_ = false;
    bool messageDialogModalActive_ = false;
    bool messageDialogModalResult_ = false;
    bool failureDiagUploadSucceeded_ = false;
    bool failureDiagResolved_ = false;
    std::wstring failureDiagKeyword_;
    std::vector<std::wstring> reExtractFailureLines_;
    size_t reExtractFailedFilesTotal_ = 0;
    int messageDialogBodyHeight_ = 0;
    int reExtractMessageHeight_ = 0;

    std::mutex uiMutex_;
    int pendingPercent_ = 0;
    bool pendingResetLog_ = false;
    std::vector<std::wstring> fileLogLines_;
    std::vector<std::wstring> fileLogDisplayLines_;
    std::vector<std::wstring> pendingFileLines_;
    int progressPercentValue_ = 0;
    std::wstring progressPercentText_ = L"0%";
    InstallHandler onInstall_;
    InstallHandler onVerify_;
    std::function<void(const std::wstring&)> onLog_;
    std::function<void()> onUpdateCheck_;
    std::function<void(const InstallerUpdateInfo&)> onApplyInstallerUpdate_;
    std::wstring lastUpdatePromptReleaseTag_;
    bool hasLastVentoyLog_ = false;
    std::wstring lastVentoyLogDrive_;
    bool lastVentoyLogFound_ = false;
    HBITMAP logoBitmap_ = nullptr;
    HICON discordFooterIcon_ = nullptr;
    std::vector<std::wstring> ventoyVersions_;
    bool ventoyVersionsLoading_ = false;
    bool ventoyVersionsLoaded_ = false;
    bool archiveMissing_ = false;
    std::wstring archiveOverridePath_;
    bool ventoyOnDrive_ = false;
    bool medicatOnDrive_ = false;
    std::wstring lastVentoyControlDrive_;
    bool hasLastMedicatLog_ = false;
    std::wstring lastMedicatLogDrive_;
    bool lastMedicatLogFound_ = false;
    unsigned lastMedicatLogScore_ = 0;
    std::atomic<bool> downloadingArchive_{false};
    bool pendingDriveRefresh_ = false;
    bool driveRefreshCoalesceFromDevice_ = false;
    std::atomic<bool> driveRefreshInFlight_{false};
    std::atomic<bool> driveRefreshCoalesce_{false};
    std::atomic<uint64_t> driveListGeneration_{0};
    std::atomic<uint64_t> ventoyStatusGeneration_{0};
    BusyProgressMode busyProgressMode_ = BusyProgressMode::None;
};

}  // namespace medicat
