#pragma once

#include <windows.h>

#include <atomic>
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
    std::wstring file;
    std::wstring statusText;
};

struct DonePayload {
    bool success = false;
    std::wstring message;
    std::wstring title;
};

enum class BusyProgressMode { FileLog, Verify, Download, None };

class Gui {
public:
    using InstallHandler = std::function<void()>;

    bool Create(HINSTANCE instance);
    int Run();
    void SetInstallHandler(InstallHandler handler);
    void SetVerifyHandler(InstallHandler handler);
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
    void OpenReExtractPrompt(ReExtractPromptPayload* payload);
    void FinishReExtractPrompt(bool wantReExtract);
    void ShowDone(bool success, const std::wstring& message, const std::wstring& title = L"");
    std::wstring SelectedDrive() const;
    bool FormatChecked() const;
    bool RunVentoyChecked() const;
    bool VentoyOnSelectedDrive() const;
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
    static LRESULT CALLBACK ReExtractWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void OnCreate(HWND hwnd);
    void OnCommand(WPARAM wp);
    void RefreshDrives();
    void RefreshDriveVentoyStatus();
    void RefreshDriveVentoyControls();
    void EnforceForcedDriveCheckboxes();
    void FlushInstallUi();
    void BatchAppendDetailLog(const std::vector<std::wstring>& files, size_t startIndex);
    void SyncDetailLog();
    void ResizeFileLogWindow(HWND hwnd);
    std::wstring FormatLogLine(size_t index, const std::wstring& path) const;
    void UpdateAdvancedControls();
    void LayoutHeader();
    void LayoutMainContent();
    void UpdateArchivePanel();
    bool IsArchiveAvailable() const;
    void EnsureVentoyVersionsLoaded();
    void PopulateVentoyVersionCombo();
    void SetVentoyVersions(std::vector<std::wstring> versions);
    void ApplyLanguageSelection(const std::wstring& languageCode);
    void RefreshTranslatedUi();
    void StartMirrorDownload(const std::wstring& url, const std::wstring& mirrorName);
    void SetDownloadControlsEnabled(bool enabled);
    void PopulateAlternativeDownloadCombo();
    void OpenSelectedAlternativeDownload();

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
    HWND progressBar_ = nullptr;
    HWND statusBar_ = nullptr;
    HWND archiveMissingLabel_ = nullptr;
    HWND downloadMirror1Btn_ = nullptr;
    HWND downloadMirror2Btn_ = nullptr;
    HWND altDownloadCombo_ = nullptr;
    HWND altDownloadOpenBtn_ = nullptr;
    HWND fileLogWindow_ = nullptr;
    HWND fileLogList_ = nullptr;
    HWND reExtractWindow_ = nullptr;
    HWND reExtractMessage_ = nullptr;
    HWND reExtractList_ = nullptr;
    HWND reExtractBtn_ = nullptr;
    HWND reExtractCloseBtn_ = nullptr;
    std::shared_ptr<ReExtractPromptState> activeReExtractPrompt_;

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
    HBITMAP logoBitmap_ = nullptr;
    std::vector<std::wstring> ventoyVersions_;
    bool ventoyVersionsLoading_ = false;
    bool ventoyVersionsLoaded_ = false;
    bool archiveMissing_ = false;
    bool ventoyOnDrive_ = false;
    std::wstring lastVentoyControlDrive_;
    std::atomic<bool> downloadingArchive_{false};
    BusyProgressMode busyProgressMode_ = BusyProgressMode::None;
};

}  // namespace medicat
