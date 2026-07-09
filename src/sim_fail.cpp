#include "sim_fail.h"

#include "i18n.h"
#include "support.h"

namespace medicat {

namespace {

std::atomic<int> g_activeSimulation{static_cast<int>(SimulatedFailure::None)};

}  // namespace

SimulatedFailure ActiveSimulatedFailure() {
    return static_cast<SimulatedFailure>(g_activeSimulation.load());
}

void SetActiveSimulatedFailure(const SimulatedFailure failure) {
    g_activeSimulation.store(static_cast<int>(failure));
}

void ClearSimulatedFailure() {
    g_activeSimulation.store(static_cast<int>(SimulatedFailure::None));
}

const wchar_t* SimulatedFailureLabel(const SimulatedFailure failure) {
    switch (failure) {
        case SimulatedFailure::VentoyDownload:
            return L"Ventoy download failed";
        case SimulatedFailure::VentoyExtract:
            return L"Ventoy extract failed";
        case SimulatedFailure::VentoyLayout:
            return L"Ventoy layout failed";
        case SimulatedFailure::VentoyRename:
            return L"Ventoy rename failed";
        case SimulatedFailure::VentoyPrepare:
            return L"Ventoy exe not found after prepare";
        case SimulatedFailure::VentoyInstall:
            return L"Ventoy install failed";
        case SimulatedFailure::VentoyUpgrade:
            return L"Ventoy upgrade failed";
        case SimulatedFailure::FormatFailed:
            return L"Format failed";
        case SimulatedFailure::MediCatExtract:
            return L"MediCat extract failed";
        case SimulatedFailure::NoInternet:
            return L"No internet";
        case SimulatedFailure::VerificationFailed:
            return L"Verification failed";
        default:
            return L"(none)";
    }
}

bool ConsumeSimulatedFailure(const SimulatedFailure expected) {
    int current = static_cast<int>(expected);
    return g_activeSimulation.compare_exchange_strong(current, static_cast<int>(SimulatedFailure::None));
}

VentoyResult MakeSimulatedVentoyFailure(const SimulatedFailure failure) {
    VentoyResult result;
    result.success = false;

    switch (failure) {
        case SimulatedFailure::VentoyDownload:
            result.failureKind = VentoyResult::FailureKind::Download;
            result.error = L"[Debug] Simulated HTTP error: connection reset by peer";
            break;
        case SimulatedFailure::VentoyExtract:
            result.failureKind = VentoyResult::FailureKind::Extract;
            result.exitCode = 2;
            result.error = L"[Debug] Simulated 7za failure\nERROR: Data Error in encrypted file";
            break;
        case SimulatedFailure::VentoyLayout:
            result.failureKind = VentoyResult::FailureKind::Layout;
            result.error = L"[Debug] Simulated layout failure\nExpected folder: ventoy-1.0.99\nContents: (empty)";
            break;
        case SimulatedFailure::VentoyRename:
            result.failureKind = VentoyResult::FailureKind::Rename;
            result.error = L"[Debug] Simulated rename failure\nFrom: ventoy-1.0.99\nTo: Ventoy2Disk\nAccess is denied (5)";
            break;
        case SimulatedFailure::VentoyPrepare:
            result.failureKind = VentoyResult::FailureKind::Prepare;
            result.error =
                L"[Debug] Simulated missing exe\nExpected: Ventoy2Disk\\Ventoy2Disk.exe\nFolder contents: (empty)";
            break;
        default:
            result.failureKind = VentoyResult::FailureKind::Prepare;
            result.error = L"[Debug] Simulated Ventoy prepare failure";
            break;
    }

    return result;
}

std::optional<SimulatedInstallFailure> MakeSimulatedInstallFailure(const SimulatedFailure failure) {
    SimulatedInstallFailure outcome;

    switch (failure) {
        case SimulatedFailure::VentoyInstall: {
            const std::wstring detail =
                L"[Debug] Simulated Ventoy2Disk install failure\nProcess exit code: 1";
            outcome.message = i18n::Tr(L"messages.ventoy_install_failed", detail);
            outcome.title = i18n::Tr(L"titles.ventoy_install_failed");
            return outcome;
        }
        case SimulatedFailure::VentoyUpgrade: {
            const std::wstring detail =
                L"[Debug] Simulated Ventoy2Disk upgrade failure\nProcess exit code: 1";
            outcome.message = i18n::Tr(L"messages.ventoy_upgrade_failed", detail);
            outcome.title = i18n::Tr(L"titles.ventoy_upgrade_failed");
            return outcome;
        }
        case SimulatedFailure::FormatFailed:
            outcome.message =
                FormatDetailedError(i18n::Tr(L"errors.format_failed", L"E:"), L"[Debug] Simulated NTFS format failure");
            outcome.title = i18n::Tr(L"titles.installation_error");
            return outcome;
        case SimulatedFailure::MediCatExtract:
            outcome.message = i18n::Tr(L"messages.extraction_failed",
                                       L"[Debug] Simulated MediCat archive extraction failure (exit code 2)");
            outcome.title = i18n::Tr(L"titles.extraction_failed");
            return outcome;
        case SimulatedFailure::NoInternet:
            outcome.message =
                FormatDetailedError(i18n::Tr(L"messages.no_internet"), L"[Debug] Simulated network unreachable");
            outcome.title = i18n::Tr(L"titles.download_failed");
            return outcome;
        case SimulatedFailure::VerificationFailed:
            outcome.message = i18n::Tr(L"messages.verification_failed", L"3");
            outcome.title = i18n::Tr(L"titles.verification_failed");
            return outcome;
        default:
            return std::nullopt;
    }
}

}  // namespace medicat
