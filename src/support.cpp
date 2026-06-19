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
#include <vector>

#pragma comment(lib, "rpcrt4.lib")

#ifndef MEDICAT_USB_VERSION
#define MEDICAT_USB_VERSION "unknown"
#endif

#ifndef MEDICAT_SESSIONS_URL
#define MEDICAT_SESSIONS_URL "https://telemetry.medicatusb.com/v1/sessions"
#endif

#ifndef MEDICAT_UPLOADS_URL
#define MEDICAT_UPLOADS_URL "https://telemetry.medicatusb.com/v1/support/uploads"
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

bool ReadPreferencesFailureLogAutoUploadEnabled() {
    const std::wstring path = GetPreferencesPath();
    if (path.empty() || !FileExists(path)) {
        return true;
    }

    std::ifstream in(WideToUtf8(path), std::ios::binary);
    if (!in) {
        return true;
    }
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (content.find("\"failure_log_auto_upload_enabled\"") == std::string::npos) {
        return true;
    }
    return content.find("\"failure_log_auto_upload_enabled\": false") == std::string::npos &&
           content.find("\"failure_log_auto_upload_enabled\":false") == std::string::npos;
}

int RunHiddenZipCommand(const std::wstring& commandLine, const std::wstring& workingDir) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = commandLine;
    const wchar_t* work = workingDir.empty() ? nullptr : workingDir.c_str();
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, work, &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(exitCode);
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
    json << "}";

    if (!request.errorTitle.empty() || !request.errorDetail.empty()) {
        json << ",\"error\":{";
        bool errorFirst = true;
        auto errorSep = [&]() {
            if (!errorFirst) {
                json << ",";
            }
            errorFirst = false;
        };
        if (!request.errorTitle.empty()) {
            errorSep();
            AppendJsonString(json, "title", request.errorTitle);
        }
        if (!request.errorDetail.empty()) {
            errorSep();
            AppendJsonString(json, "detail", request.errorDetail);
        }
        json << "}";
    }

    json << "}";

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

std::vector<std::wstring> CollectSupportLogFiles(const std::wstring& installerRoot) {
    static const wchar_t* kAllowed[] = {L"medicat_installer.log", L"extract.log",      L"reextract.log",
                                          L"check.log",            L"failed_files.txt", nullptr};
    std::vector<std::wstring> files;
    for (const wchar_t* const* name = kAllowed; *name != nullptr; ++name) {
        const std::wstring path = JoinPath(installerRoot, *name);
        if (FileExists(path) && GetFileSizeBytes(path) > 0) {
            files.push_back(*name);
        }
    }
    return files;
}

void AppendJsonStringValue(std::ostringstream& json, const std::string& value) {
    json << "\"";
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

std::string BuildSupportManifestJson(const FailureLogUploadRequest& request,
                                     const std::vector<std::wstring>& filesIncluded) {
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
    AppendJsonString(json, "session_id", request.sessionId);
    sep();
    AppendJsonString(json, "client", "MedicatInstaller");
    sep();
    AppendJsonString(json, "operation", request.operation);
    sep();
    AppendJsonString(json, "installer_version", kInstallerVersion);
    sep();
    AppendJsonInt(json, "installer_build", kInstallerBuildNumber);
    if (!request.errorTitle.empty()) {
        sep();
        AppendJsonString(json, "error_title", request.errorTitle);
    }
    if (!request.errorDetail.empty()) {
        sep();
        AppendJsonString(json, "error_detail", request.errorDetail);
    }

    json << ",\"files_included\":[";
    for (size_t i = 0; i < filesIncluded.size(); ++i) {
        if (i > 0) {
            json << ",";
        }
        AppendJsonStringValue(json, WideToUtf8(filesIncluded[i]));
    }
    json << "]}";
    return json.str();
}

bool CreateSupportLogZip(const FailureLogUploadRequest& request, const std::vector<std::wstring>& logFiles,
                         const std::wstring& manifestPath, const std::wstring& zipPath) {
    (void)manifestPath;
    if (request.sevenZa.empty() || logFiles.empty()) {
        return false;
    }

    std::wstring command = L"\"" + request.sevenZa + L"\" a -tzip -y \"" + zipPath + L"\"";
    for (const std::wstring& name : logFiles) {
        command += L" \"" + name + L"\"";
    }
    command += L" \"support_manifest.json\"";

    const int exitCode = RunHiddenZipCommand(command, request.installerRoot);
    return exitCode == 0 && FileExists(zipPath) && GetFileSizeBytes(zipPath) > 0;
}

void UploadFailureLogsOnce(const FailureLogUploadRequest& request, const SessionReportLogger& logLine) {
    if (!HasIngestToken()) {
        LogTelemetry(logLine, L"Failure log upload skipped — no ingest token configured at build time");
        return;
    }
    if (!ReadPreferencesFailureLogAutoUploadEnabled()) {
        LogTelemetry(logLine, L"Failure log upload skipped — disabled in preferences");
        return;
    }
    if (request.sessionId.empty() || request.installerRoot.empty() || request.sevenZa.empty()) {
        LogTelemetry(logLine, L"Failure log upload skipped — missing session or tools");
        return;
    }

    std::vector<std::wstring> logFiles = CollectSupportLogFiles(request.installerRoot);
    if (logFiles.empty()) {
        LogTelemetry(logLine, L"Failure log upload skipped — no log files found", true);
        return;
    }

    const std::string manifestJson = BuildSupportManifestJson(request, logFiles);
    const std::wstring manifestPath = JoinPath(request.installerRoot, L"support_manifest.json");
    {
        std::ofstream manifestOut(WideToUtf8(manifestPath), std::ios::binary);
        if (!manifestOut) {
            LogTelemetry(logLine, L"Failure log upload skipped — could not write manifest", true);
            return;
        }
        manifestOut.write(manifestJson.data(), static_cast<std::streamsize>(manifestJson.size()));
    }

    const std::wstring zipPath = JoinPath(GetMedicatTempDir(), L"support_upload.zip");
    DeleteFileW(zipPath.c_str());
    if (!CreateSupportLogZip(request, logFiles, manifestPath, zipPath)) {
        DeleteFileW(manifestPath.c_str());
        LogTelemetry(logLine, L"Failure log upload skipped — could not create zip bundle", true);
        return;
    }

    LogTelemetry(logLine, L"Failure log upload attempted — " + std::to_wstring(logFiles.size()) + L" file(s)");
    const HttpMultipartResult upload = HttpPostMultipartUpload(
        Utf8ToWide(MEDICAT_UPLOADS_URL), Utf8ToWide(GetIngestToken()), Utf8ToWide(request.sessionId), manifestJson,
        zipPath);

    DeleteFileW(manifestPath.c_str());
    DeleteFileW(zipPath.c_str());

    if (upload.statusCode >= 200 && upload.statusCode < 300) {
        LogTelemetry(logLine, L"Failure logs accepted by server (HTTP " + std::to_wstring(upload.statusCode) +
                                   L")" + (upload.keyword.empty() ? L"" : L" — keyword " + upload.keyword));
        return;
    }

    LogTelemetry(logLine, L"Failure log upload failed — " +
                               (upload.error.empty() ? L"HTTP " + std::to_wstring(upload.statusCode) : upload.error),
                 true);
}

}  // namespace

std::wstring FormatDetailedError(const std::wstring& summary, const std::wstring& detail) {
    if (summary.empty()) {
        return detail;
    }
    if (detail.empty() || detail == summary) {
        return summary;
    }
    return summary + L"\n\n" + detail;
}

std::string SanitizeTelemetryText(const std::wstring& text, const size_t maxLen) {
    if (text.empty()) {
        return {};
    }

    std::wstring sanitized;
    sanitized.reserve(text.size());

    for (size_t i = 0; i < text.size();) {
        const wchar_t ch = text[i];

        if (ch == L'\\' && i + 2 < text.size() && text[i + 1] == L'?' && text[i + 2] == L'\\') {
            sanitized += L"<path>\\";
            i += 3;
            continue;
        }

        if (((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z')) && i + 1 < text.size() && text[i + 1] == L':') {
            sanitized += L"<drive>:";
            i += 2;
            if (i < text.size() && text[i] == L'\\') {
                sanitized += L'\\';
                ++i;
            }
            continue;
        }

        if (ch == L'\\' && i + 6 < text.size() && (text.compare(i, 7, L"\\Users\\") == 0 ||
                                                   text.compare(i, 7, L"\\users\\") == 0)) {
            sanitized += L"\\Users\\<user>\\";
            i += 7;
            continue;
        }

        sanitized.push_back(ch);
        ++i;
    }

    std::string utf8 = medicat::WideToUtf8(sanitized);
    if (utf8.size() > maxLen) {
        utf8.resize(maxLen);
    }
    return utf8;
}

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

bool FailureLogAutoUploadEnabled() {
    return ReadPreferencesFailureLogAutoUploadEnabled();
}

void SendFailureLogUpload(const FailureLogUploadRequest& request, SessionReportLogger logLine) {
    LogTelemetry(logLine, L"Failure log upload queued (background upload)");
    std::thread worker(UploadFailureLogsOnce, request, logLine);
    worker.detach();
}

}  // namespace medicat
