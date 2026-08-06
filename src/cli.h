#pragma once

#include <optional>
#include <string>
#include <vector>

namespace medicat {

enum class CliAction {
    None,
    Help,
    Version,
    ListDrives,
    DumpConfig,
    Install,
    Verify,
};

enum class CliReextractPolicy {
    Default,
    Reextract,
    NoReextract,
    ReextractOnly,
};

struct CliOptions {
    CliAction action = CliAction::None;
    std::wstring drive;
    std::wstring language;
    std::wstring archivePath;
    std::wstring logPath;

    std::optional<bool> format;
    std::optional<bool> runVentoy;
    std::optional<bool> gpt;
    std::optional<bool> secureBoot;
    std::wstring ventoyVersion;

    bool allowFixed = false;
    bool yes = false;
    bool quiet = false;
    bool offlineOnly = false;
    CliReextractPolicy reextract = CliReextractPolicy::Default;
};

struct CliParseResult {
    CliOptions options;
    bool ok = true;
    int errorCode = 0;
    std::wstring errorMessage;
};

CliParseResult ParseCommandLine(int argc, wchar_t** argv);

bool AttachCliConsole();
void WriteCliLine(const std::wstring& line);
void WriteCliUtf8(const std::string& line);
void WriteCliTip(const std::wstring& line);
void WriteCliProgress(const std::wstring& line);
void WriteCliProgressFinish();
std::wstring FormatCliFileProgress(int percent, const std::wstring& file);

void PrintCliHelp();
void PrintCliVersion();
void PrintCliDrives(bool allowFixed);
void PrintCliConfig(const std::wstring& root, const std::wstring& sevenZa, const std::wstring& md5Manifest,
                    const std::wstring& archivePath);

std::wstring NormalizeDriveLetter(const std::wstring& drive);
bool IsSupportedLanguage(const std::wstring& code);

}  // namespace medicat
