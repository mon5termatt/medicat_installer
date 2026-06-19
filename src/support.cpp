#include "support.h"

#include "cancel.h"
#include "download.h"
#include "i18n.h"
#include "ingest_token.h"
#include "util.h"

#include <windows.h>
#include <knownfolders.h>
#include <rpc.h>
#include <shlobj.h>

#include <fstream>
#include <sstream>
#include <thread>

#pragma comment(lib, "rpcrt4.lib")

#ifndef MEDICAT_USB_VERSION
#define MEDICAT_USB_VERSION "unknown"
#endif

#ifndef MEDICAT_SESSIONS_URL
#define MEDICAT_SESSIONS_URL "https://telemetry.medicatusb.com/v1/sessions"
#endif

#ifndef MEDICAT_RELEASE_TAG
#define MEDICAT_RELEASE_TAG ""
#endif

namespace medicat {

namespace {

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) {
        return {};
    }
    std::string out(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), len, nullptr, nullptr);
    return out;
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (len <= 1) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), len);
    return wide;
}

void AppendJsonString(std::ostringstream& json, const std::string& key, const std::string& value) {
    json << "\"" << key << "\":\"";
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                json << "\\\\";
                break;
            case '"':
                json << "\\\"";
                break;
            case '\n':
                json << "\\n";
                break;
            case '\r':
                json << "\\r";
                break;
            case '\t':
                json << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    continue;
                }
                json << ch;
                break;
        }
    }
    json << "\"";
}

void AppendJsonInt(std::ostringstream& json, const std::string& key, int64_t value) {
    json << "\"" << key << "\":" << value;
}

void AppendJsonBool(std::ostringstream& json, const std::string& key, bool value) {
    json << "\"" << key << "\":" << (value ? "true" : "false");
}

std::wstring GetPreferencesPath() {
    PWSTR roaming = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming)) || !roaming) {
        return {};
    }
    std::wstring path = JoinPath(roaming, L"MedicatInstaller");
    CoTaskMemFree(roaming);
    return JoinPath(path, L"preferences.json");
}

bool ReadPreferencesSessionReportsEnabled() {
    const std::wstring path = GetPreferencesPath();
    if (path.empty() || !FileExists(path)) {
        return true;
    }

    std::ifstream in(WideToUtf8(path), std::ios::binary);
    if (!in) {
        return true;
    }
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (content.find("\"session_reports_enabled\"") == std::string::npos) {
        return true;
    }
    return content.find("\"session_reports_enabled\": false") == std::string::npos &&
           content.find("\"session_reports_enabled\":false") == std::string::npos;
}

std::string InstallerArchLabel() {
#if defined(_M_X64)
    return "x64";
#elif defined(_M_IX86)
    return "x86";
#elif defined(_M_ARM64)
    return "arm64";
#else
    return "unknown";
#endif
}

std::string BuildSessionReportJson(const SessionReportRequest& request) {
    const SessionSystemSnapshot system = CollectSessionSystemSnapshot();

    std::ostringstream json;
    json << "{";
    bool first = true;
    auto sep = [&]() {
        if (!first) {
            json << ",";
        }
        first = false;
    };

    sep();
    AppendJsonInt(json, "schema_version", 1);
    sep();
    AppendJsonString(json, "session_id", request.sessionId);
    sep();
    AppendJsonString(json, "client", "MedicatInstaller");
    sep();
    AppendJsonString(json, "operation", request.operation);
    sep();
    AppendJsonString(json, "outcome", request.outcome);
    sep();
    AppendJsonInt(json, "exit_code", request.exitCode);
    sep();
    AppendJsonString(json, "installer_version", kInstallerVersion);
    sep();
    AppendJsonInt(json, "installer_build", kInstallerBuildNumber);
    sep();
    AppendJsonString(json, "installer_arch", InstallerArchLabel());
    sep();
    AppendJsonString(json, "medicat_usb_version", MEDICAT_USB_VERSION);
    if (MEDICAT_RELEASE_TAG[0] != '\0') {
        sep();
        AppendJsonString(json, "release_tag", MEDICAT_RELEASE_TAG);
    }
    sep();
    AppendJsonInt(json, "duration_ms", request.durationMs);
    if (!system.locale.empty()) {
        sep();
        AppendJsonString(json, "locale", system.locale);
    }
    sep();
    AppendJsonBool(json, "elevated", IsProcessElevated());

    json << ",\"options\":{";
    bool optionsFirst = true;
    auto optionSep = [&]() {
        if (!optionsFirst) {
            json << ",";
        }
        optionsFirst = false;
    };
    optionSep();
    AppendJsonBool(json, "format", request.diagnostic.formatChecked);
    optionSep();
    AppendJsonBool(json, "ventoy", request.diagnostic.runVentoyChecked);
    optionSep();
    AppendJsonBool(json, "ventoy_gpt", request.diagnostic.ventoyGpt);
    optionSep();
    AppendJsonBool(json, "ventoy_secure_boot", request.diagnostic.ventoySecureBoot);
    optionSep();
    AppendJsonBool(json, "headless", request.headless);
    json << "}";

    json << ",\"system\":{";
    bool systemFirst = true;
    auto systemSep = [&]() {
        if (!systemFirst) {
            json << ",";
        }
        systemFirst = false;
    };
    if (system.windowsBuild > 0) {
        systemSep();
        AppendJsonInt(json, "windows_build", system.windowsBuild);
    }
    if (!system.windowsMajorMinor.empty()) {
        systemSep();
        AppendJsonString(json, "windows_major_minor", system.windowsMajorMinor);
    }
    if (!system.editionId.empty()) {
        systemSep();
        AppendJsonString(json, "edition_id", system.editionId);
    }
    if (!system.installationType.empty()) {
        systemSep();
        AppendJsonString(json, "installation_type", system.installationType);
    }
    if (!system.processorArch.empty()) {
        systemSep();
        AppendJsonString(json, "processor_arch", system.processorArch);
    }
    if (system.logicalProcessors > 0) {
        systemSep();
        AppendJsonInt(json, "logical_processors", system.logicalProcessors);
    }
    if (!system.ramGbBucket.empty()) {
        systemSep();
        AppendJsonString(json, "ram_gb_bucket", system.ramGbBucket);
    }
    json << "}}";

    return json.str();
}

void LogTelemetry(const SessionReportLogger& logLine, const std::wstring& message, const bool isError = false) {
    if (logLine) {
        logLine(message, isError);
    }
}

std::wstring TelemetrySessionLabel(const std::string& sessionId) {
    if (sessionId.size() <= 8) {
        return Utf8ToWide(sessionId);
    }
    return Utf8ToWide(sessionId.substr(0, 8)) + L"…";
}

void PostSessionReportOnce(const SessionReportRequest& request, const SessionReportLogger& logLine) {
    if (!HasIngestToken()) {
        LogTelemetry(logLine, L"Session report skipped — no ingest token configured at build time");
        return;
    }

    const std::string ingestToken = GetIngestToken();
    const std::wstring sessionLabel = TelemetrySessionLabel(request.sessionId);
    LogTelemetry(logLine, L"Session report upload attempted — target " + Utf8ToWide(MEDICAT_SESSIONS_URL) +
                               L", session " + sessionLabel + L", operation=" + Utf8ToWide(request.operation) +
                               L", outcome=" + Utf8ToWide(request.outcome));

    const std::string body = BuildSessionReportJson(request);
    std::wstring error;
    const int status = HttpPostJson(Utf8ToWide(MEDICAT_SESSIONS_URL), body, Utf8ToWide(ingestToken), error);

    if (status >= 200 && status < 300) {
        LogTelemetry(logLine, L"Session report accepted by server (HTTP " + std::to_wstring(status) +
                                   L") — ingest token OK");
        return;
    }

    if (status == 401) {
        LogTelemetry(logLine, L"Session report rejected — ingest token unauthorized (HTTP 401)", true);
        return;
    }

    if (status == 503) {
        LogTelemetry(logLine, L"Session report rejected — server ingest not configured (HTTP 503)", true);
        return;
    }

    if (status == 0) {
        LogTelemetry(logLine, L"Session report failed — " + (error.empty() ? L"network or HTTP error" : error), true);
        return;
    }

    LogTelemetry(logLine, L"Session report rejected (HTTP " + std::to_wstring(status) + L")" +
                               (error.empty() ? L"" : L": " + error),
                 true);
}

}  // namespace

std::string GenerateSessionId() {
    UUID uuid{};
    if (UuidCreate(&uuid) != RPC_S_OK) {
        return "00000000-0000-0000-0000-000000000000";
    }

    char buffer[37];
    sprintf_s(buffer, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid.Data1, uuid.Data2, uuid.Data3,
              uuid.Data4[0], uuid.Data4[1], uuid.Data4[2], uuid.Data4[3], uuid.Data4[4], uuid.Data4[5],
              uuid.Data4[6], uuid.Data4[7]);
    return buffer;
}

std::string DeriveSessionOutcome(const bool success, const std::wstring& message, const std::wstring& title,
                                 const std::wstring& operation) {
    if (IsCancelRequested()) {
        return "cancelled";
    }
    if (!success && message.empty()) {
        return "cancelled";
    }
    if (success) {
        return "success";
    }

    if (title == i18n::Tr(L"titles.verify_still_failed_after_reextract")) {
        return "verify_failed_after_reextract";
    }
    if (title == i18n::Tr(L"titles.verification_failed")) {
        return "verify_failed";
    }
    if (title == i18n::Tr(L"titles.verification_error")) {
        return "verify_error";
    }
    if (title == i18n::Tr(L"titles.re_extraction_error")) {
        return "reextract_failed";
    }
    if (operation == L"verify") {
        return "verify_failed";
    }
    return "install_failed";
}

bool SessionReportsEnabled() {
    return ReadPreferencesSessionReportsEnabled();
}

void SendSessionReport(const SessionReportRequest& request, const bool waitForCompletion,
                       SessionReportLogger logLine) {
    if (!SessionReportsEnabled()) {
        LogTelemetry(logLine, L"Session report skipped — disabled in preferences");
        return;
    }
    if (request.sessionId.empty()) {
        LogTelemetry(logLine, L"Session report skipped — no session id");
        return;
    }

    if (waitForCompletion) {
        PostSessionReportOnce(request, logLine);
        return;
    }

    LogTelemetry(logLine, L"Session report queued (background upload)");
    std::thread worker(PostSessionReportOnce, request, logLine);
    worker.detach();
}

}  // namespace medicat
