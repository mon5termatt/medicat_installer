#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace medicat {

void EnableConsoleCancelHandling();
void ResetCancelState();
bool IsCancelRequested();
void RequestCancel();

class ChildProcessRegistration {
public:
    explicit ChildProcessRegistration(HANDLE process);
    ~ChildProcessRegistration();

    ChildProcessRegistration(const ChildProcessRegistration&) = delete;
    ChildProcessRegistration& operator=(const ChildProcessRegistration&) = delete;

private:
    HANDLE process_ = nullptr;
};

}  // namespace medicat
