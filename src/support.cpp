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

bool CopyFileShared(const std::wstring& sourcePath, const std::wstring& destinationPath) {
    const HANDLE input = CreateFileW(sourcePath.c_str(), GENERIC_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
    if (input == INVALID_HANDLE_VALUE) {
        return false;
    }

    const HANDLE output = CreateFileW(destinationPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE) {
        CloseHandle(input);
        return false;
    }

    std::vector<BYTE> buffer(64 * 1024);
    bool ok = true;
    for (;;) {
        DWORD read = 0;
        if (!ReadFile(input, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr)) {
            ok = false;
            break;
        }
        if (read == 0) {
            break;
        }

        DWORD written = 0;
        if (!WriteFile(output, buffer.data(), read, &written, nullptr) || written != read) {
            ok = false;
            break;
        }
    }

    CloseHandle(output);
    CloseHandle(input);
    return ok;
}

bool ClearDirectoryFiles(const std::wstring& directory) {
    const std::wstring pattern = JoinPath(directory, L"*");
    WIN32_FIND_DATAW fd{};
    const HANDLE find = FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return GetLastError() == ERROR_FILE_NOT_FOUND;
    }

    do {
        const std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        const std::wstring fullPath = JoinPath(directory, name);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        SetFileAttributesW(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(fullPath.c_str());
    } while (FindNextFileW(find, &fd));

    FindClose(find);
    return true;
}

bool StageSupportLogFiles(const std::wstring& installerRoot, const std::vector<std::wstring>& logFiles,
                          const std::wstring& stagingDir, std::vector<std::wstring>& stagedFiles) {
    CreateDirectoryW(stagingDir.c_str(), nullptr);
    ClearDirectoryFiles(stagingDir);
    stagedFiles.clear();

    for (const std::wstring& name : logFiles) {
        const std::wstring sourcePath = JoinPath(installerRoot, name);
        const std::wstring destinationPath = JoinPath(stagingDir, name);
        if (!CopyFileShared(sourcePath, destinationPath)) {
            continue;
        }
        stagedFiles.push_back(name);
    }

    return !stagedFiles.empty();
}

bool ZipCommandSucceeded(const int exitCode, const std::wstring& zipPath) {
    return FileExists(zipPath) && GetFileSizeBytes(zipPath) > 0 && (exitCode == 0 || exitCode == 1);
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
    sep();
    AppendJsonString(json, "ui_language", WideToUtf8(i18n::CurrentLanguage()));
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
    if (system.windowsUbr > 0) {
        systemSep();
        AppendJsonInt(json, "windows_ubr", system.windowsUbr);
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
    if (!system.machineIdHash.empty()) {
        systemSep();
        AppendJsonString(json, "machine_id_hash", system.machineIdHash);
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
    sep();
    AppendJsonString(json, "ui_language", WideToUtf8(i18n::CurrentLanguage()));
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

bool CreateSupportLogZip(const FailureLogUploadRequest& request, const std::wstring& stagingDir,
                         const std::vector<std::wstring>& stagedFiles, const std::wstring& zipPath,
                         int& exitCodeOut) {
    exitCodeOut = -1;
    if (request.sevenZa.empty() || stagedFiles.empty()) {
        return false;
    }

    std::wstring command = L"\"" + request.sevenZa + L"\" a -tzip -y \"" + zipPath + L"\"";
    for (const std::wstring& name : stagedFiles) {
        command += L" \"" + JoinPath(stagingDir, name) + L"\"";
    }
    command += L" \"" + JoinPath(stagingDir, L"support_manifest.json") + L"\"";

    exitCodeOut = RunHiddenZipCommand(command, stagingDir);
    return ZipCommandSucceeded(exitCodeOut, zipPath);
}

void UploadFailureLogsOnce(const FailureLogUploadRequest& request, const SessionReportLogger& logLine,
                           const FailureLogUploadCompleteCallback& onComplete) {
    auto finish = [&](const bool success, const std::wstring& keyword) {
        if (onComplete) {
            onComplete(success, keyword);
        }
    };
    if (!HasIngestToken()) {
        LogTelemetry(logLine, L"Failure log upload skipped — no ingest token configured at build time");
        finish(false, L"");
        return;
    }
    if (!ReadPreferencesFailureLogAutoUploadEnabled()) {
        LogTelemetry(logLine, L"Failure log upload skipped — disabled in preferences");
        finish(false, L"");
        return;
    }
    if (request.sessionId.empty() || request.installerRoot.empty() || request.sevenZa.empty()) {
        LogTelemetry(logLine, L"Failure log upload skipped — missing session or tools");
        finish(false, L"");
        return;
    }

    std::vector<std::wstring> logFiles = CollectSupportLogFiles(request.installerRoot);
    if (logFiles.empty()) {
        LogTelemetry(logLine, L"Failure log upload skipped — no log files found", true);
        finish(false, L"");
        return;
    }

    const std::wstring stagingDir = JoinPath(GetMedicatTempDir(), L"support_upload_staging");
    std::vector<std::wstring> stagedFiles;
    if (!StageSupportLogFiles(request.installerRoot, logFiles, stagingDir, stagedFiles)) {
        LogTelemetry(logLine, L"Failure log upload skipped — could not stage log files for upload", true);
        finish(false, L"");
        return;
    }

    const std::string manifestJson = BuildSupportManifestJson(request, stagedFiles);
    const std::wstring manifestPath = JoinPath(stagingDir, L"support_manifest.json");
    {
        std::ofstream manifestOut(WideToUtf8(manifestPath), std::ios::binary);
        if (!manifestOut) {
            LogTelemetry(logLine, L"Failure log upload skipped — could not write manifest", true);
            finish(false, L"");
            return;
        }
        manifestOut.write(manifestJson.data(), static_cast<std::streamsize>(manifestJson.size()));
    }

    const std::wstring zipPath = JoinPath(GetMedicatTempDir(), L"support_upload.zip");
    DeleteFileW(zipPath.c_str());
    int zipExitCode = -1;
    if (!CreateSupportLogZip(request, stagingDir, stagedFiles, zipPath, zipExitCode)) {
        LogTelemetry(logLine, L"Failure log upload skipped — could not create zip bundle (7za exit " +
                                   std::to_wstring(zipExitCode) + L")",
                     true);
        finish(false, L"");
        return;
    }

    const uint64_t zipBytes = GetFileSizeBytes(zipPath);
    LogTelemetry(logLine, L"Failure log upload attempted — " + std::to_wstring(stagedFiles.size()) +
                               L" file(s), " + std::to_wstring(zipBytes) + L" bytes, target " +
                               Utf8ToWide(MEDICAT_UPLOADS_URL));
    const HttpMultipartResult upload = HttpPostMultipartUpload(
        Utf8ToWide(MEDICAT_UPLOADS_URL), Utf8ToWide(GetIngestToken()), Utf8ToWide(request.sessionId), manifestJson,
        zipPath);

    DeleteFileW(zipPath.c_str());
    ClearDirectoryFiles(stagingDir);

    if (upload.statusCode >= 200 && upload.statusCode < 300) {
        LogTelemetry(logLine, L"Failure logs accepted by server (HTTP " + std::to_wstring(upload.statusCode) +
                                   L")" + (upload.keyword.empty() ? L"" : L" — keyword " + upload.keyword));
        finish(!upload.keyword.empty(), upload.keyword);
        return;
    }

    LogTelemetry(logLine, L"Failure log upload failed — " +
                               (upload.error.empty() ? L"HTTP " + std::to_wstring(upload.statusCode) : upload.error),
                 true);
    finish(false, L"");
}

}  // namespace

namespace {

bool IsWindowsDriveLetterColon(const std::wstring& text, const size_t index) {
    if (index + 1 >= text.size() || text[index + 1] != L':') {
        return false;
    }

    const wchar_t letter = text[index];
    const bool isLetter = (letter >= L'A' && letter <= L'Z') || (letter >= L'a' && letter <= L'z');
    if (!isLetter) {
        return false;
    }

    if (index > 0) {
        const wchar_t previous = text[index - 1];
        if (previous != L'\\' && previous != L'/' && previous != L' ' && previous != L'\t' && previous != L'\n' &&
            previous != L'\r') {
            return false;
        }
    }

    if (index + 2 < text.size()) {
        const wchar_t next = text[index + 2];
        if (next != L'\\' && next != L'/') {
            return false;
        }
    }

    return true;
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

        if (IsWindowsDriveLetterColon(text, i)) {
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

std::string SanitizeTelemetryTextEnglish(const std::wstring& text, const size_t maxLen) {
    return SanitizeTelemetryText(i18n::ToEnglish(text), maxLen);
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
    if (title == i18n::Tr(L"titles.medicat_not_on_drive") ||
        title == i18n::Tr(L"titles.verification_all_failed_wrong_drive")) {
        return "verify_wrong_drive";
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

void SendFailureLogUpload(const FailureLogUploadRequest& request, SessionReportLogger logLine,
                          FailureLogUploadCompleteCallback onComplete) {
    LogTelemetry(logLine, L"Failure log upload queued (background upload)");
    std::thread worker(
        [](FailureLogUploadRequest uploadRequest, SessionReportLogger uploadLogLine,
           FailureLogUploadCompleteCallback completeCallback) {
            UploadFailureLogsOnce(uploadRequest, uploadLogLine, std::move(completeCallback));
        },
        request, std::move(logLine), std::move(onComplete));
    worker.detach();
}

}  // namespace medicat
