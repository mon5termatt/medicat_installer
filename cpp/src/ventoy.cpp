#include "ventoy.h"

#include "util.h"

#include <windows.h>

#include <sstream>
#include <vector>

namespace medicat {

namespace {

int RunHiddenProcess(const std::wstring& commandLine, const std::wstring& workingDir = L"") {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> cmd(commandLine.begin(), commandLine.end());
    cmd.push_back(L'\0');

    PROCESS_INFORMATION pi{};
    const wchar_t* work = workingDir.empty() ? nullptr : workingDir.c_str();
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, work,
                        &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(code);
}

}  // namespace

VentoyResult RunVentoyInstall(const std::wstring& ventoyExe, const std::wstring& driveLetter,
                              const bool upgrade) {
    VentoyResult result;
    if (!FileExists(ventoyExe)) {
        result.error = L"Ventoy2Disk.exe not found";
        return result;
    }

    std::wstring drive = driveLetter;
    if (drive.size() >= 2 && drive[1] == L':') {
        drive = drive.substr(0, 2);
    }

    const std::wstring ventoyDir = GetExeDirectory();
    const std::wstring ventoyWork = JoinPath(ventoyDir, L"Ventoy2Disk");
    const std::wstring args =
        upgrade ? (L"VTOYCLI /U /Drive:" + drive) : (L"VTOYCLI /I /Drive:" + drive + L" /NOUSBCheck");

    std::wstring cmd = L"\"" + ventoyExe + L"\" " + args;
    result.exitCode = RunHiddenProcess(cmd, ventoyWork);
    result.success = (result.exitCode == 0);
    if (!result.success) {
        std::wostringstream err;
        err << L"Ventoy failed with exit code " << result.exitCode;
        result.error = err.str();
    }
    return result;
}

bool FormatDriveNtfs(const std::wstring& driveLetter, const std::wstring& label) {
    std::wstring drive = driveLetter;
    if (drive.size() == 2 && drive[1] == L':') {
        drive += L':';
    }
    std::wstring cmd =
        L"format.com " + drive + L" /FS:NTFS /X /Q /V:" + label + L" /Y";
    return RunHiddenProcess(cmd) == 0;
}

}  // namespace medicat
