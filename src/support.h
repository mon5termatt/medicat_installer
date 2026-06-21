#pragma once

#include "debug.h"

#include <chrono>
#include <functional>
#include <string>

namespace medicat {

struct SessionReportRequest {
    std::string sessionId;
    std::string operation;
    std::string outcome;
    int exitCode = 0;
    int64_t durationMs = 0;
    bool headless = false;
    std::string errorTitle;
    std::string errorDetail;
    DiagnosticContext diagnostic;
};

std::wstring FormatDetailedError(const std::wstring& summary, const std::wstring& detail);
std::string SanitizeTelemetryText(const std::wstring& text, size_t maxLen = 512);
// Sanitize user-facing text for upload: translate to English first, then redact paths.
std::string SanitizeTelemetryTextEnglish(const std::wstring& text, size_t maxLen = 512);

std::string GenerateSessionId();
std::string DeriveSessionOutcome(bool success, const std::wstring& message, const std::wstring& title,
                                 const std::wstring& operation);
bool SessionReportsEnabled();
using SessionReportLogger = std::function<void(const std::wstring& message, bool isError)>;
void SendSessionReport(const SessionReportRequest& request, bool waitForCompletion,
                       SessionReportLogger logLine = nullptr);

struct FailureLogUploadRequest {
    std::string sessionId;
    std::string operation;
    std::wstring installerRoot;
    std::wstring sevenZa;
    std::string errorTitle;
    std::string errorDetail;
    DiagnosticContext diagnostic;
};

bool FailureLogAutoUploadEnabled();
using FailureLogUploadCompleteCallback = std::function<void(bool success, const std::wstring& keyword)>;
void SendFailureLogUpload(const FailureLogUploadRequest& request, SessionReportLogger logLine = nullptr,
                          FailureLogUploadCompleteCallback onComplete = nullptr);

}  // namespace medicat
