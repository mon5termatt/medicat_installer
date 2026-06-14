#pragma once

#include <windows.h>

#include <string>

namespace medicat::theme {

struct Palette {
    COLORREF window = RGB(30, 30, 34);
    COLORREF panel = RGB(30, 30, 34);
    COLORREF control = RGB(44, 44, 50);
    COLORREF buttonHover = RGB(54, 54, 60);
    COLORREF border = RGB(58, 58, 66);
    COLORREF text = RGB(232, 232, 236);
    COLORREF muted = RGB(140, 144, 154);
    COLORREF accent = RGB(56, 189, 168);
    COLORREF accentHover = RGB(72, 204, 182);
    COLORREF accentPressed = RGB(44, 160, 142);
    COLORREF progressTrack = RGB(44, 44, 50);
    COLORREF progressFill = RGB(56, 189, 168);
};

struct Brushes {
    HBRUSH window = nullptr;
    HBRUSH panel = nullptr;
    HBRUSH control = nullptr;
};

bool Initialize();
void Shutdown();
const Palette& Colors();
const Brushes& GetBrushes();
HBITMAP LoadLogoBitmap(HINSTANCE instance, int maxSize = 48);
HFONT MakeUiFont();
HFONT MakeTitleFont();
HFONT MakeSubtitleFont();
HFONT MakeLogFont();
void EnableDarkMode(HWND hwnd);
void EnableDarkModeRecursive(HWND root);
void PaintProgressBar(HDC hdc, const RECT& rc, int percent, const std::wstring& text, HFONT font);

enum class ButtonStyle { Primary, Secondary };

enum class ButtonState { Normal, Hovered, Pressed, Disabled };

void PaintFlatButton(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, ButtonStyle style,
                     ButtonState state);

void PaintFlatCheckbox(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, bool checked, bool enabled);

}  // namespace medicat::theme
