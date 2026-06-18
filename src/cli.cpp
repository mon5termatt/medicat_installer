#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "cli.h"

#include "drives.h"
#include "i18n.h"
#include "i18n_generated.h"
#include "util.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <iostream>
#include <sstream>
#include <string>

#ifndef MEDICAT_USB_VERSION
#define MEDICAT_USB_VERSION "unknown"
#endif

namespace medicat {

namespace {

std::wstring TrimWhitespace(std::wstring text) {
    while (!text.empty() && iswspace(text.front())) {
        text.erase(text.begin());
    }
    while (!text.empty() && iswspace(text.back())) {
        text.pop_back();
    }
    return text;
}

std::wstring StripQuotes(std::wstring text) {
    text = TrimWhitespace(std::move(text));
    if (text.size() >= 2 && text.front() == L'"' && text.back() == L'"') {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

std::wstring NormalizeFlagName(std::wstring token) {
    if (token.size() >= 2 && token[0] == L'-' && token[1] == L'-') {
        token.erase(0, 2);
    } else if (!token.empty() && token[0] == L'/') {
        token.erase(0, 1);
    }
    std::transform(token.begin(), token.end(), token.begin(), towlower);
    return token;
}

bool SplitFlagValue(const std::wstring& token, std::wstring& name, std::wstring& value, bool& hasValue) {
    hasValue = false;
    value.clear();

    std::wstring body = token;
    if (body.size() >= 2 && body[0] == L'-' && body[1] == L'-') {
        body.erase(0, 2);
        const size_t eq = body.find(L'=');
        if (eq != std::wstring::npos) {
            name = body.substr(0, eq);
            value = StripQuotes(body.substr(eq + 1));
            hasValue = true;
            return true;
        }
        name = body;
        return true;
    }

    if (!body.empty() && body[0] == L'/') {
        body.erase(0, 1);
    }

    const size_t colon = body.find(L':');
    if (colon != std::wstring::npos) {
        name = body.substr(0, colon);
        value = StripQuotes(body.substr(colon + 1));
        hasValue = !value.empty();
        return true;
    }

    const size_t eq = body.find(L'=');
    if (eq != std::wstring::npos) {
        name = body.substr(0, eq);
        value = StripQuotes(body.substr(eq + 1));
        hasValue = !value.empty();
        return true;
    }

    name = body;
    return true;
}

bool IsFlagToken(const std::wstring& token) {
    return token.size() >= 2 && ((token[0] == L'/' && token[1] != L' ') || (token[0] == L'-' && token[1] == L'-'));
}

void FailParse(CliParseResult& result, int code, const std::wstring& message) {
    result.ok = false;
    result.errorCode = code;
    result.errorMessage = message;
}

bool SetTriState(std::optional<bool>& target, bool value, CliParseResult& result, const wchar_t* positive,
                 const wchar_t* negative) {
    if (target.has_value()) {
        FailParse(result, 2, std::wstring(L"Conflicting flags: ") + positive + L" and " + negative);
        return false;
    }
    target = value;
    return true;
}

}  // namespace

bool AttachCliConsole() {
    if (GetConsoleWindow() != nullptr) {
        return true;
    }
    if (!AllocConsole()) {
        return false;
    }

    wchar_t title[128];
    swprintf_s(title, L"MediCat Installer v%hs", kInstallerVersion);
    SetConsoleTitleW(title);

    FILE* dummy = nullptr;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$", "r", stdin);

    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != nullptr && out != INVALID_HANDLE_VALUE) {
        constexpr SHORT kWidth = 120;
        constexpr SHORT kWindowHeight = 40;
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (GetConsoleScreenBufferInfo(out, &info)) {
            const COORD buffer = {kWidth, static_cast<SHORT>(info.dwSize.Y > 9999 ? info.dwSize.Y : 9999)};
            if (SetConsoleScreenBufferSize(out, buffer)) {
                const SMALL_RECT window = {0, 0, static_cast<SHORT>(kWidth - 1), kWindowHeight - 1};
                SetConsoleWindowInfo(out, TRUE, &window);
            }
        }
    }
    return true;
}

void WriteCliLine(const std::wstring& line) {
    std::wcout << line << std::endl;
}

void WriteCliUtf8(const std::string& line) {
    std::cout << line << std::endl;
}

namespace {

HANDLE CliStdoutHandle() {
    AttachCliConsole();
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

void WriteConsoleRaw(const std::wstring& text) {
    const HANDLE out = CliStdoutHandle();
    if (out == nullptr || out == INVALID_HANDLE_VALUE) {
        WriteCliLine(text);
        return;
    }
    DWORD written = 0;
    WriteConsoleW(out, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

WORD SaveConsoleAttributes(const HANDLE out) {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (out == nullptr || out == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(out, &info)) {
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
    return info.wAttributes;
}

void RestoreConsoleAttributes(const HANDLE out, const WORD attributes) {
    if (out != nullptr && out != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(out, attributes);
    }
}

std::wstring PadConsoleLine(const HANDLE out, std::wstring line) {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (out == nullptr || out == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(out, &info)) {
        return line;
    }

    const int width = info.srWindow.Right - info.srWindow.Left + 1;
    if (width > 0 && static_cast<int>(line.size()) < width) {
        line.append(static_cast<size_t>(width - line.size()), L' ');
    }
    return line;
}

int CliConsoleWidthChars() {
    const HANDLE out = CliStdoutHandle();
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (out != nullptr && out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(out, &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
    }
    return 120;
}

}  // namespace

std::wstring FormatCliFileProgress(const int percent, const std::wstring& file) {
    const std::wstring percentStr = std::to_wstring(percent);
    const std::wstring prefix = i18n::Tr(L"status.extracting_file", percentStr, L"");
    const int consoleWidth = CliConsoleWidthChars();
    size_t maxFileLen = 48;
    if (consoleWidth > static_cast<int>(prefix.size())) {
        maxFileLen = static_cast<size_t>(consoleWidth - prefix.size());
    }
    const std::wstring displayFile = ShortDisplayPath(file, maxFileLen);
    return i18n::Tr(L"status.extracting_file", percentStr, displayFile);
}

void WriteCliTip(const std::wstring& line) {
    const HANDLE out = CliStdoutHandle();
    const WORD oldAttributes = SaveConsoleAttributes(out);
    SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_INTENSITY);
    WriteConsoleRaw(line);
    WriteConsoleRaw(L"\r\n");
    RestoreConsoleAttributes(out, oldAttributes);
}

void WriteCliProgress(const std::wstring& line) {
    const HANDLE out = CliStdoutHandle();
    const WORD oldAttributes = SaveConsoleAttributes(out);
    SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    WriteConsoleRaw(L"\r");
    WriteConsoleRaw(PadConsoleLine(out, line));
    RestoreConsoleAttributes(out, oldAttributes);
}

void WriteCliProgressFinish() {
    WriteConsoleRaw(L"\r\n");
}

void PrintCliHelp() {
    AttachCliConsole();
    WriteCliUtf8(R"(MediCat USB Installer — usage

  MedicatInstaller.exe                     Open graphical installer
  MedicatInstaller.exe /help               Show this help
  MedicatInstaller.exe /version            Show version and build

Actions:
  /install /drive:E                        Install MediCat to drive E:
  /verify /drive:E                         Verify MD5 hashes on drive E:

Common options:
  /format /noformat                        NTFS format before extract
  /ventoy /noventoy                        Install or update Ventoy
  /gpt /secureboot /nosb                   Ventoy partition options
  /ventoy-version:1.1.12                   Pin Ventoy release
  /archive:"D:\path\MediCat.USB.v21.12.7z" Override archive location
  /lang:en                                 UI language (en es fr pl tr)
  /yes                                     Accept destructive prompts (required with /quiet)
  /quiet                                   No dialogs; use exit codes
  /offline                                 Use offline Ventoy/archive cache only
  /allow-fixed                             Include fixed HDD/SSD drives (>= 30 GiB)
  /reextract /noreextract                  Control selective re-extract on verify failure

Diagnostics:
  /list-drives                             List eligible removable/VHD drives
  /dump-config                             Show resolved paths and options

Exit codes: 0 ok, 1 error, 2 bad args, 3 need admin, 4 cancelled, 5 verify failed, 6 re-extract incomplete

Administrator required for /install. Logs: medicat_installer.log beside the exe.)");
}

void PrintCliVersion() {
    AttachCliConsole();
    std::ostringstream out;
#if defined(_WIN64)
    out << "MedicatInstaller " << kInstallerVersion << " (build " << kInstallerBuildNumber << ") x64\n";
#else
    out << "MedicatInstaller " << kInstallerVersion << " (build " << kInstallerBuildNumber << ") x86\n";
#endif
    out << "Release tag: (not embedded)\n";
    out << "MediCat USB: v" << MEDICAT_USB_VERSION;
    WriteCliUtf8(out.str());
}

void PrintCliDrives(const bool allowFixed) {
    AttachCliConsole();
    const std::vector<DriveInfo> drives = ListTargetDrives(allowFixed);
    if (drives.empty()) {
        WriteCliLine(L"(no eligible drives)");
        return;
    }

    for (const DriveInfo& drive : drives) {
        std::wostringstream line;
        line << drive.letter;
        if (!drive.label.empty()) {
            line << L" \"" << drive.label << L'"';
        }
        line << L" (" << drive.kind << L") - " << FormatBytes(drive.freeBytes) << L" free of "
             << FormatBytes(drive.totalBytes);
        WriteCliLine(line.str());
    }
}

void PrintCliConfig(const std::wstring& root, const std::wstring& sevenZa, const std::wstring& md5Manifest,
                    const std::wstring& archivePath) {
    AttachCliConsole();
    WriteCliLine(L"Installer directory: " + root);
    WriteCliLine(L"7za: " + (sevenZa.empty() ? L"(missing)" : sevenZa));
    WriteCliLine(L"MD5 manifest: " + (md5Manifest.empty() ? L"(missing)" : md5Manifest));
    WriteCliLine(L"MediCat archive: " + archivePath);
    WriteCliLine(L"Temp dir: " + GetMedicatTempDir());
}

std::wstring NormalizeDriveLetter(const std::wstring& drive) {
    if (drive.empty()) {
        return {};
    }

    std::wstring normalized = TrimWhitespace(drive);
    while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L'/')) {
        normalized.pop_back();
    }
    if (normalized.empty()) {
        return {};
    }

    wchar_t letter = towupper(normalized[0]);
    if (letter < L'A' || letter > L'Z') {
        return {};
    }
    if (normalized.size() == 1 || (normalized.size() >= 2 && normalized[1] == L':')) {
        return std::wstring(1, letter) + L":";
    }
    return {};
}

bool IsSupportedLanguage(const std::wstring& code) {
    for (size_t i = 0; i < i18n::generated::SupportedLanguageCount; ++i) {
        if (code == i18n::generated::SupportedLanguages[i]) {
            return true;
        }
    }
    return false;
}

CliParseResult ParseCommandLine(int argc, wchar_t** argv) {
    CliParseResult result;

    for (int i = 1; i < argc; ++i) {
        const std::wstring token = argv[i];
        if (!IsFlagToken(token)) {
            FailParse(result, 2, L"Unexpected argument: " + token);
            return result;
        }

        std::wstring name;
        std::wstring value;
        bool hasValue = false;
        if (!SplitFlagValue(token, name, value, hasValue)) {
            FailParse(result, 2, L"Invalid flag: " + token);
            return result;
        }

        name = NormalizeFlagName(name);

        if (name == L"help" || name == L"h" || name == L"?") {
            result.options.action = CliAction::Help;
            continue;
        }
        if (name == L"version" || name == L"v") {
            result.options.action = CliAction::Version;
            continue;
        }
        if (name == L"list-drives") {
            result.options.action = CliAction::ListDrives;
            continue;
        }
        if (name == L"dump-config") {
            result.options.action = CliAction::DumpConfig;
            continue;
        }
        if (name == L"install") {
            if (result.options.action == CliAction::Verify) {
                FailParse(result, 2, L"/install and /verify cannot be used together");
                return result;
            }
            result.options.action = CliAction::Install;
            continue;
        }
        if (name == L"verify") {
            if (result.options.action == CliAction::Install) {
                FailParse(result, 2, L"/install and /verify cannot be used together");
                return result;
            }
            result.options.action = CliAction::Verify;
            continue;
        }

        if (name == L"drive") {
            if (!hasValue) {
                if (i + 1 >= argc || IsFlagToken(argv[i + 1])) {
                    FailParse(result, 2, L"/drive requires a drive letter");
                    return result;
                }
                value = argv[++i];
                hasValue = true;
            }
            result.options.drive = NormalizeDriveLetter(value);
            if (result.options.drive.empty()) {
                FailParse(result, 2, L"Invalid drive letter: " + value);
                return result;
            }
            continue;
        }

        if (name == L"lang") {
            if (!hasValue) {
                if (i + 1 >= argc || IsFlagToken(argv[i + 1])) {
                    FailParse(result, 2, L"/lang requires a language code");
                    return result;
                }
                value = argv[++i];
            }
            const std::wstring lang = TrimWhitespace(value);
            if (!IsSupportedLanguage(lang)) {
                FailParse(result, 2, L"Unsupported language: " + lang);
                return result;
            }
            result.options.language = lang;
            continue;
        }

        if (name == L"archive") {
            if (!hasValue) {
                if (i + 1 >= argc || IsFlagToken(argv[i + 1])) {
                    FailParse(result, 2, L"/archive requires a path");
                    return result;
                }
                value = argv[++i];
            }
            result.options.archivePath = StripQuotes(value);
            continue;
        }

        if (name == L"log") {
            if (!hasValue) {
                if (i + 1 >= argc || IsFlagToken(argv[i + 1])) {
                    FailParse(result, 2, L"/log requires a path");
                    return result;
                }
                value = argv[++i];
            }
            result.options.logPath = StripQuotes(value);
            continue;
        }

        if (name == L"ventoy-version") {
            if (!hasValue) {
                if (i + 1 >= argc || IsFlagToken(argv[i + 1])) {
                    FailParse(result, 2, L"/ventoy-version requires a version");
                    return result;
                }
                value = argv[++i];
            }
            result.options.ventoyVersion = TrimWhitespace(value);
            continue;
        }

        if (name == L"format") {
            if (!SetTriState(result.options.format, true, result, L"/format", L"/noformat")) {
                return result;
            }
            continue;
        }
        if (name == L"noformat") {
            if (!SetTriState(result.options.format, false, result, L"/format", L"/noformat")) {
                return result;
            }
            continue;
        }
        if (name == L"ventoy") {
            if (!SetTriState(result.options.runVentoy, true, result, L"/ventoy", L"/noventoy")) {
                return result;
            }
            continue;
        }
        if (name == L"noventoy") {
            if (!SetTriState(result.options.runVentoy, false, result, L"/ventoy", L"/noventoy")) {
                return result;
            }
            continue;
        }
        if (name == L"gpt") {
            if (!SetTriState(result.options.gpt, true, result, L"/gpt", L"/nogpt")) {
                return result;
            }
            continue;
        }
        if (name == L"nogpt") {
            if (!SetTriState(result.options.gpt, false, result, L"/gpt", L"/nogpt")) {
                return result;
            }
            continue;
        }
        if (name == L"secureboot") {
            if (!SetTriState(result.options.secureBoot, true, result, L"/secureboot", L"/nosb")) {
                return result;
            }
            continue;
        }
        if (name == L"nosb" || name == L"nosecureboot") {
            if (!SetTriState(result.options.secureBoot, false, result, L"/secureboot", L"/nosb")) {
                return result;
            }
            continue;
        }

        if (name == L"yes" || name == L"y") {
            result.options.yes = true;
            continue;
        }
        if (name == L"quiet" || name == L"q") {
            result.options.quiet = true;
            continue;
        }
        if (name == L"offline") {
            result.options.offlineOnly = true;
            continue;
        }
        if (name == L"allow-fixed") {
            result.options.allowFixed = true;
            continue;
        }

        if (name == L"reextract") {
            if (result.options.reextract != CliReextractPolicy::Default) {
                FailParse(result, 2, L"Conflicting re-extract flags");
                return result;
            }
            result.options.reextract = CliReextractPolicy::Reextract;
            continue;
        }
        if (name == L"noreextract") {
            if (result.options.reextract != CliReextractPolicy::Default) {
                FailParse(result, 2, L"Conflicting re-extract flags");
                return result;
            }
            result.options.reextract = CliReextractPolicy::NoReextract;
            continue;
        }
        if (name == L"reextract-only") {
            if (result.options.reextract != CliReextractPolicy::Default) {
                FailParse(result, 2, L"Conflicting re-extract flags");
                return result;
            }
            result.options.reextract = CliReextractPolicy::ReextractOnly;
            continue;
        }

        FailParse(result, 2, L"Unknown flag: " + token);
        return result;
    }

    if (result.options.action == CliAction::Help) {
        return result;
    }
    if (result.options.action == CliAction::Version) {
        return result;
    }
    if (result.options.action == CliAction::ListDrives) {
        return result;
    }
    if (result.options.action == CliAction::DumpConfig) {
        return result;
    }

    if (result.options.action == CliAction::Install || result.options.action == CliAction::Verify) {
        if (result.options.drive.empty()) {
            FailParse(result, 2, L"/drive is required for /install and /verify");
            return result;
        }
        if (result.options.quiet && !result.options.yes &&
            result.options.action == CliAction::Install) {
            FailParse(result, 2, L"/quiet requires /yes for /install");
            return result;
        }
        return result;
    }

    if (result.options.action == CliAction::None && argc > 1) {
        bool onlyLang = true;
        for (int i = 1; i < argc; ++i) {
            const std::wstring token = argv[i];
            std::wstring name;
            std::wstring value;
            bool hasValue = false;
            SplitFlagValue(token, name, value, hasValue);
            name = NormalizeFlagName(name);
            if (name != L"lang") {
                onlyLang = false;
                break;
            }
        }
        if (!onlyLang) {
            FailParse(result, 2, L"Unknown or incomplete flags; use /help for usage");
            return result;
        }
    }

    return result;
}

}  // namespace medicat
