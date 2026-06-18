#include "app.h"

#include "cli.h"
#include "i18n.h"

#include <shellapi.h>
#include <windows.h>

namespace medicat {

int RunApp(HINSTANCE instance) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    const CliParseResult parsed = ParseCommandLine(argc, argv);
    if (!parsed.ok) {
        AttachCliConsole();
        WriteCliLine(parsed.errorMessage);
        WriteCliLine(L"Use /help for usage.");
        if (argv) {
            LocalFree(argv);
        }
        return parsed.errorCode != 0 ? parsed.errorCode : 2;
    }

    if (!parsed.options.language.empty()) {
        i18n::Load(parsed.options.language);
    }

    App app(instance, parsed.options.logPath);
    const int code = app.RunParsed(parsed, argc, argv);
    if (argv) {
        LocalFree(argv);
    }
    return code;
}

}  // namespace medicat

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    return medicat::RunApp(instance);
}
