#include "failure_tracker.h"

#include "util.h"

#include <shlobj.h>

#include <string>

namespace medicat {

namespace {

constexpr wchar_t kAppFolder[] = L"MedicatInstaller";
constexpr wchar_t kStateFile[] = L"operation_failures.txt";

std::wstring FailureStatePath() {
    PWSTR localAppData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData)) || !localAppData) {
        return L"";
    }
    const std::wstring dir = JoinPath(localAppData, kAppFolder);
    CoTaskMemFree(localAppData);
    CreateDirectoryW(dir.c_str(), nullptr);
    return JoinPath(dir, kStateFile);
}

int ReadFailureCount() {
    const std::wstring path = FailureStatePath();
    if (path.empty() || !FileExists(path)) {
        return 0;
    }

    const HANDLE file =
        CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    char buffer[32]{};
    DWORD read = 0;
    const BOOL ok = ReadFile(file, buffer, static_cast<DWORD>(sizeof(buffer) - 1), &read, nullptr);
    CloseHandle(file);
    if (!ok || read == 0) {
        return 0;
    }

    buffer[read] = '\0';
    try {
        const int value = std::stoi(buffer);
        return value < 0 ? 0 : value;
    } catch (...) {
        return 0;
    }
}

void WriteFailureCount(const int count) {
    const std::wstring path = FailureStatePath();
    if (path.empty()) {
        return;
    }

    const std::string text = std::to_string(count < 0 ? 0 : count);
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                    nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD written = 0;
    WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
    CloseHandle(file);
}

}  // namespace

int OperationFailureCount() {
    return ReadFailureCount();
}

void RecordOperationFailure() {
    WriteFailureCount(ReadFailureCount() + 1);
}

void RecordOperationSuccess() {
    WriteFailureCount(0);
}

bool NeedsHelpGate() {
    return ReadFailureCount() >= kHelpGateFailureThreshold;
}

}  // namespace medicat
