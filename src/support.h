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
    DiagnosticContext diagnostic;
};

std::string GenerateSessionId();
std::string DeriveSessionOutcome(bool success, const std::wstring& message, const std::wstring& title,
                                 const std::wstring& operation);
bool SessionReportsEnabled();
using SessionReportLogger = std::function<void(const std::wstring& message, bool isError)>;
void SendSessionReport(const SessionReportRequest& request, bool waitForCompletion,
                       SessionReportLogger logLine = nullptr);

}  // namespace medicat
