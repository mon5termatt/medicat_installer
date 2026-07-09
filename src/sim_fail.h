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

}  // namespace medicat
