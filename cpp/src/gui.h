#pragma once

#include <windows.h>

#include <functional>
#include <string>

namespace medicat {

constexpr UINT WM_MEDICAT_PROGRESS = WM_APP + 1;
constexpr UINT WM_MEDICAT_DONE = WM_APP + 2;

struct ProgressPayload {
    int percent = 0;
    std::wstring status;
};

struct DonePayload {
    bool success = false;
    std::wstring message;
};

class Gui {
public:
    using InstallHandler = std::function<void()>;

    bool Create(HINSTANCE instance);
    int Run();
    void SetInstallHandler(InstallHandler handler);
    void SetBusy(bool busy);
    void SetProgress(int percent, const std::wstring& status);
    void ShowDone(bool success, const std::wstring& message);
    std::wstring SelectedDrive() const;
    bool FormatChecked() const;
    bool SkipVentoyChecked() const;
    HWND Hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void OnCreate(HWND hwnd);
    void OnCommand(WPARAM wp);
    void RefreshDrives();

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND driveCombo_ = nullptr;
    HWND formatCheck_ = nullptr;
    HWND skipVentoyCheck_ = nullptr;
    HWND installBtn_ = nullptr;
    HWND progressBar_ = nullptr;
    HWND statusLabel_ = nullptr;
    InstallHandler onInstall_;
};

}  // namespace medicat
