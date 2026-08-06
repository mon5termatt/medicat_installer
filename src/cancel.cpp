#include <atomic>
#include <cstdio>
#include <mutex>
#include <vector>

#include "cancel.h"
#include "cli.h"

namespace medicat {

namespace {

std::mutex g_childMutex;
std::vector<HANDLE> g_childProcesses;
std::atomic<bool> g_cancelRequested{false};
std::atomic<bool> g_handlerInstalled{false};

BOOL WINAPI ConsoleCtrlHandler(const DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        RequestCancel();
        if (type == CTRL_CLOSE_EVENT) {
            ExitProcess(4);
        }
        return TRUE;
    }
    return FALSE;
}

}  // namespace

void EnableConsoleCancelHandling() {
    if (g_handlerInstalled.exchange(true)) {
        return;
    }
    AttachCliConsole();
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void ResetCancelState() {
    g_cancelRequested.store(false);
}

bool IsCancelRequested() {
    return g_cancelRequested.load();
}

void RequestCancel() {
    g_cancelRequested.store(true);

    std::lock_guard lock(g_childMutex);
    for (HANDLE process : g_childProcesses) {
        TerminateProcess(process, 1);
    }
}

ChildProcessRegistration::ChildProcessRegistration(const HANDLE process) {
    if (!process || process == INVALID_HANDLE_VALUE) {
        return;
    }

    HANDLE duplicate = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &duplicate, 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
        return;
    }

    {
        std::lock_guard lock(g_childMutex);
        g_childProcesses.push_back(duplicate);
    }
    process_ = duplicate;
}

ChildProcessRegistration::~ChildProcessRegistration() {
    if (!process_) {
        return;
    }

    std::lock_guard lock(g_childMutex);
    for (auto it = g_childProcesses.begin(); it != g_childProcesses.end(); ++it) {
        if (*it == process_) {
            g_childProcesses.erase(it);
            break;
        }
    }
    CloseHandle(process_);
    process_ = nullptr;
}

}  // namespace medicat
