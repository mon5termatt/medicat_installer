#pragma once

#include <windows.h>

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace medicat {

constexpr UINT WM_MEDICAT_PROGRESS = WM_APP + 1;
constexpr UINT WM_MEDICAT_DONE = WM_APP + 2;
constexpr UINT WM_MEDICAT_VENTOY_VERSIONS = WM_APP + 3;

struct ProgressPayload {
    int percent = 0;
    bool clearLog = false;
    bool resetLog = false;
    bool extractUpdate = false;
    std::wstring file;
};

struct DonePayload {
    bool success = false;
    std::wstring message;
    std::wstring title;
};

enum class BusyProgressMode { FileLog, Verify, None };

class Gui {
public:
    using InstallHandler = std::function<void()>;

    bool Create(HINSTANCE instance);
    int Run();
    void SetInstallHandler(InstallHandler handler);
    void SetVerifyHandler(InstallHandler handler);
    void SetBusy(bool busy, BusyProgressMode progressMode = BusyProgressMode::FileLog);
    void SetProgress(int percent, bool clearLog = false);
    void SetCurrentFileLabel(const std::wstring& text);
    void NotifyExtractProgress(int percent, const std::wstring& file = L"", bool resetLog = false);
    void ClearFileLog();
    void OpenFileLogWindow();
    void ShowDone(bool success, const std::wstring& message, const std::wstring& title = L"");
    std::wstring SelectedDrive() const;
    bool FormatChecked() const;
    bool SkipVentoyChecked() const;
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
    void OnCreate(HWND hwnd);
    void OnCommand(WPARAM wp);
    void RefreshDrives();
    void FlushInstallUi();
    void BatchAppendDetailLog(const std::vector<std::wstring>& files, size_t startIndex);
    void SyncDetailLog();
    void ResizeFileLogWindow(HWND hwnd);
    std::wstring FormatLogLine(size_t index, const std::wstring& path) const;
    void UpdateAdvancedControls();
    void LayoutVersionLabel();
    void EnsureVentoyVersionsLoaded();
    void PopulateVentoyVersionCombo();
    void SetVentoyVersions(std::vector<std::wstring> versions);

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND logoStatic_ = nullptr;
    HWND titleLabel_ = nullptr;
    HWND versionLabel_ = nullptr;
    HWND driveCombo_ = nullptr;
    HWND showAllDrivesCheck_ = nullptr;
    HWND formatCheck_ = nullptr;
    HWND skipVentoyCheck_ = nullptr;
    HWND advancedCheck_ = nullptr;
    HWND pinVentoyCheck_ = nullptr;
    HWND ventoySecureBootCheck_ = nullptr;
    HWND ventoyGptCheck_ = nullptr;
    HWND ventoyVersionCombo_ = nullptr;
    HWND installBtn_ = nullptr;
    HWND verifyFilesBtn_ = nullptr;
    HWND openLogBtn_ = nullptr;
    HWND progressBar_ = nullptr;
    HWND currentFileLabel_ = nullptr;
    HWND fileLogWindow_ = nullptr;
    HWND fileLogList_ = nullptr;

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
    BusyProgressMode busyProgressMode_ = BusyProgressMode::None;
};

}  // namespace medicat
