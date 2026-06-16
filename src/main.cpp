#include "app.hpp"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    medicat::App app(instance);
    return app.Run();
}
