#pragma once

#include "ventoy.h"

#include <atomic>
#include <optional>
#include <string>
#include <utility>

namespace medicat {

enum class SimulatedFailure {
    None = 0,
    VentoyDownload,
    VentoyExtract,
    VentoyLayout,
    VentoyRename,
    VentoyPrepare,
    VentoyInstall,
    VentoyUpgrade,
    FormatFailed,
    MediCatExtract,
    NoInternet,
    VerificationFailed,
};

SimulatedFailure ActiveSimulatedFailure();
void SetActiveSimulatedFailure(SimulatedFailure failure);
void ClearSimulatedFailure();
const wchar_t* SimulatedFailureLabel(SimulatedFailure failure);

// Returns true and clears the armed failure when it matches expected.
bool ConsumeSimulatedFailure(SimulatedFailure expected);

VentoyResult MakeSimulatedVentoyFailure(SimulatedFailure failure);

struct SimulatedInstallFailure {
    std::wstring message;
    std::wstring title;
};

std::optional<SimulatedInstallFailure> MakeSimulatedInstallFailure(SimulatedFailure failure);

// Session-lived Shift+logo safety toggles (not one-shot; stay until toggled off).
struct DebugSafetyFlags {
    bool skipArchiveValidation = false;   // skip MediCat archive MD5 (file must still exist / look complete)
    bool skipDestructiveConfirms = false; // wipe + Ventoy warning prompts
    bool skipPresenceCheck = false;       // skip MediCat-on-drive gate before verify
};

DebugSafetyFlags& DebugSafety();
bool ToggleDebugSafetyFlag(int flagId);  // returns new value; flagId is DebugSafetyMenuId
const wchar_t* DebugSafetyFlagLabel(int flagId);

enum class DebugSafetyMenuId {
    SkipArchiveValidation = 1,
    SkipDestructiveConfirms = 2,
    SkipPresenceCheck = 3,
};

}  // namespace medicat
