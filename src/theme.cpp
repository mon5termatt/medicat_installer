#include "theme.hpp"

#include "resource.hpp"

#include <objidl.h>
#include <algorithm>
#include <vector>
#include <gdiplus.h>
#include <wincodec.h>
#include <dwmapi.h>
#include <uxtheme.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace medicat::theme {

namespace {

Palette g_palette;
Brushes g_brushes;
bool g_initialized = false;
ULONG_PTR g_gdiplusToken = 0;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

void AddRoundedRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, const float radius) {
    const float d = radius * 2.0f;
    if (d <= 0.0f || rect.Width < d || rect.Height < d) {
        path.AddRectangle(rect);
        return;
    }

    path.AddArc(rect.X, rect.Y, d, d, 180.0f, 90.0f);
    path.AddArc(rect.GetRight() - d, rect.Y, d, d, 270.0f, 90.0f);
    path.AddArc(rect.GetRight() - d, rect.GetBottom() - d, d, d, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.GetBottom() - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

HBITMAP CreateHBitmapFromWicBitmap(IWICBitmapSource* source) {
    if (!source) {
        return nullptr;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(source->GetSize(&width, &height)) || width == 0 || height == 0) {
        return nullptr;
    }

    IWICImagingFactory* factory = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
        return nullptr;
    }

    IWICFormatConverter* converter = nullptr;
    HRESULT hr = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                   WICBitmapPaletteTypeMedianCut);
    }

    IWICBitmapSource* bitmapSource = source;
    if (SUCCEEDED(hr)) {
        bitmapSource = converter;
    }

    const UINT stride = width * 4;
    const UINT bufferSize = stride * height;
    std::vector<BYTE> pixels(bufferSize);
    hr = bitmapSource->CopyPixels(nullptr, stride, bufferSize, pixels.data());

    if (converter) {
        converter->Release();
    }
    factory->Release();

    if (FAILED(hr)) {
        return nullptr;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(width);
    bmi.bmiHeader.biHeight = -static_cast<LONG>(height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);
    if (!bitmap || !bits) {
        return nullptr;
    }

    std::vector<BYTE> bgraPixels(bufferSize);
    memcpy(bgraPixels.data(), pixels.data(), bufferSize);
    for (size_t i = 0; i < bgraPixels.size(); i += 4) {
        const BYTE alpha = bgraPixels[i + 3];
        if (alpha != 0) {
            bgraPixels[i] = static_cast<BYTE>((bgraPixels[i] * alpha + 127) / 255);
            bgraPixels[i + 1] = static_cast<BYTE>((bgraPixels[i + 1] * alpha + 127) / 255);
            bgraPixels[i + 2] = static_cast<BYTE>((bgraPixels[i + 2] * alpha + 127) / 255);
        }
    }

    memcpy(bits, bgraPixels.data(), bufferSize);
    return bitmap;
}

}  // namespace

bool Initialize() {
    if (g_initialized) {
        return true;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    Gdiplus::GdiplusStartupInput gdiplusInput;
    if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok) {
        return false;
    }

    g_brushes.window = CreateSolidBrush(g_palette.window);
    g_brushes.panel = CreateSolidBrush(g_palette.panel);
    g_brushes.control = CreateSolidBrush(g_palette.control);
    g_initialized = g_brushes.window && g_brushes.panel && g_brushes.control;
    return g_initialized;
}

void Shutdown() {
    if (g_brushes.window) {
        DeleteObject(g_brushes.window);
        g_brushes.window = nullptr;
    }
    if (g_brushes.panel) {
        DeleteObject(g_brushes.panel);
        g_brushes.panel = nullptr;
    }
    if (g_brushes.control) {
        DeleteObject(g_brushes.control);
        g_brushes.control = nullptr;
    }
    g_initialized = false;
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
    CoUninitialize();
}

const Palette& Colors() { return g_palette; }

const Brushes& GetBrushes() { return g_brushes; }

HBITMAP LoadLogoBitmap(const HINSTANCE instance, const int maxSize) {
    HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(IDR_LOGO_PNG), RT_RCDATA);
    if (!resource) {
        return nullptr;
    }

    const HGLOBAL loaded = LoadResource(instance, resource);
    if (!loaded) {
        return nullptr;
    }

    const void* data = LockResource(loaded);
    const DWORD size = SizeofResource(instance, resource);
    if (!data || size == 0) {
        return nullptr;
    }

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!mem) {
        return nullptr;
    }

    void* dest = GlobalLock(mem);
    if (!dest) {
        GlobalFree(mem);
        return nullptr;
    }
    memcpy(dest, data, size);
    GlobalUnlock(mem);

    IStream* stream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(mem, TRUE, &stream))) {
        GlobalFree(mem);
        return nullptr;
    }

    auto* src = Gdiplus::Bitmap::FromStream(stream);
    stream->Release();
    if (!src || src->GetLastStatus() != Gdiplus::Ok) {
        delete src;
        return nullptr;
    }

    const INT srcW = src->GetWidth();
    const INT srcH = src->GetHeight();
    if (srcW <= 0 || srcH <= 0) {
        delete src;
        return nullptr;
    }

    const int limit = std::max(1, maxSize);
    const float scale =
        std::min(static_cast<float>(limit) / static_cast<float>(srcW),
                 static_cast<float>(limit) / static_cast<float>(srcH));
    const int dstW = std::max(1, static_cast<int>(static_cast<float>(srcW) * scale + 0.5f));
    const int dstH = std::max(1, static_cast<int>(static_cast<float>(srcH) * scale + 0.5f));

    Gdiplus::Bitmap dst(dstW, dstH, PixelFormat32bppPARGB);
    Gdiplus::Graphics graphics(&dst);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
    graphics.DrawImage(src, 0, 0, dstW, dstH);
    delete src;

    HBITMAP bitmap = nullptr;
    if (dst.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &bitmap) != Gdiplus::Ok) {
        return nullptr;
    }
    return bitmap;
}

bool PaintLogo(HDC hdc, const HINSTANCE instance, const RECT& bounds, const int maxSize) {
    if (!hdc || !instance) {
        return false;
    }

    HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(IDR_LOGO_PNG), RT_RCDATA);
    if (!resource) {
        return false;
    }

    const HGLOBAL loaded = LoadResource(instance, resource);
    if (!loaded) {
        return false;
    }

    const void* data = LockResource(loaded);
    const DWORD size = SizeofResource(instance, resource);
    if (!data || size == 0) {
        return false;
    }

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!mem) {
        return false;
    }

    void* dest = GlobalLock(mem);
    if (!dest) {
        GlobalFree(mem);
        return false;
    }
    memcpy(dest, data, size);
    GlobalUnlock(mem);

    IStream* stream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(mem, TRUE, &stream))) {
        GlobalFree(mem);
        return false;
    }

    auto* bitmap = Gdiplus::Bitmap::FromStream(stream);
    stream->Release();
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
        delete bitmap;
        return false;
    }

    const INT srcW = bitmap->GetWidth();
    const INT srcH = bitmap->GetHeight();
    if (srcW <= 0 || srcH <= 0) {
        delete bitmap;
        return false;
    }

    const int limit = std::max(1, maxSize);
    const float scale =
        std::min(static_cast<float>(limit) / static_cast<float>(srcW),
                 static_cast<float>(limit) / static_cast<float>(srcH));
    const int dstW = std::max(1, static_cast<int>(static_cast<float>(srcW) * scale + 0.5f));
    const int dstH = std::max(1, static_cast<int>(static_cast<float>(srcH) * scale + 0.5f));

    Gdiplus::Bitmap scaled(dstW, dstH, PixelFormat32bppPARGB);
    Gdiplus::Graphics scaledGraphics(&scaled);
    scaledGraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    scaledGraphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    scaledGraphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
    scaledGraphics.DrawImage(bitmap, 0, 0, dstW, dstH);
    delete bitmap;

    Gdiplus::Graphics target(hdc);
    target.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
    target.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    target.DrawImage(&scaled, bounds.left, bounds.top, width > 0 ? width : dstW, height > 0 ? height : dstH);
    return true;
}

HICON LoadLogoIcon(const HINSTANCE instance, const int size) {
    const int iconSize = std::max(1, size);
    HICON icon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                               iconSize, iconSize, LR_DEFAULTCOLOR | LR_SHARED));
    if (!icon) {
        icon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    }
    return icon;
}

HFONT MakeUiFont() {
    return CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

HFONT MakeTitleFont() {
    return CreateFontW(-22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Semibold");
}

HFONT MakeSubtitleFont() {
    return CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

HFONT MakeLogFont() {
    return CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Cascadia Mono");
}

void EnableDarkMode(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
}

void EnableDarkModeRecursive(HWND root) {
    EnableDarkMode(root);
    for (HWND child = GetWindow(root, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        EnableDarkModeRecursive(child);
    }
}

void PaintProgressBar(HDC hdc, const RECT& rc, const int percent, const std::wstring& text, HFONT font) {
    if (rc.right <= rc.left || rc.bottom <= rc.top) {
        return;
    }

    RECT inner = rc;
    InflateRect(&inner, -1, -1);

    const int width = inner.right - inner.left;
    int fillWidth = MulDiv(std::clamp(percent, 0, 100), width, 100);
    if (fillWidth < 0) {
        fillWidth = 0;
    }
    if (fillWidth > width) {
        fillWidth = width;
    }

    const HBRUSH track = CreateSolidBrush(g_palette.progressTrack);
    FillRect(hdc, &inner, track);
    DeleteObject(track);

    if (fillWidth > 0) {
        RECT filled = inner;
        filled.right = inner.left + fillWidth;
        const HBRUSH fill = CreateSolidBrush(g_palette.progressFill);
        FillRect(hdc, &filled, fill);
        DeleteObject(fill);
    }

    const HPEN border = CreatePen(PS_SOLID, 1, g_palette.border);
    const HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, border));
    const HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, GetStockObject(HOLLOW_BRUSH)));
    Rectangle(hdc, inner.left, inner.top, inner.right, inner.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(border);

    HFONT oldFont = nullptr;
    if (font) {
        oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, font));
    }

    SetBkMode(hdc, TRANSPARENT);

    const int saved = SaveDC(hdc);
    IntersectClipRect(hdc, inner.left, inner.top, inner.left + fillWidth, inner.bottom);
    SetTextColor(hdc, RGB(18, 18, 22));
    DrawTextW(hdc, text.c_str(), -1, &inner, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    RestoreDC(hdc, saved);

    const int saved2 = SaveDC(hdc);
    IntersectClipRect(hdc, inner.left + fillWidth, inner.top, inner.right, inner.bottom);
    SetTextColor(hdc, g_palette.text);
    DrawTextW(hdc, text.c_str(), -1, &inner, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    RestoreDC(hdc, saved2);

    if (oldFont) {
        SelectObject(hdc, oldFont);
    }
}

void PaintFlatButton(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, const ButtonStyle style,
                     const ButtonState state) {
    if (!hdc || rc.right <= rc.left || rc.bottom <= rc.top) {
        return;
    }

    const bool enabled = state != ButtonState::Disabled;
    const bool primary = style == ButtonStyle::Primary;
    const float radius = primary ? 6.0f : 6.0f;

    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);

    const Gdiplus::RectF bodyRect(static_cast<Gdiplus::REAL>(rc.left), static_cast<Gdiplus::REAL>(rc.top),
                                  static_cast<Gdiplus::REAL>(rc.right - rc.left),
                                  static_cast<Gdiplus::REAL>(rc.bottom - rc.top));

    const Gdiplus::Color windowColor(255, GetRValue(g_palette.window), GetGValue(g_palette.window),
                                     GetBValue(g_palette.window));
    Gdiplus::SolidBrush windowBrush(windowColor);
    graphics.FillRectangle(&windowBrush, bodyRect);

    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);

    Gdiplus::GraphicsPath bodyPath;
    AddRoundedRect(bodyPath, bodyRect, radius);

    Gdiplus::Color fillColor;
    Gdiplus::Color borderColor;
    Gdiplus::Color textColor;

    if (!enabled) {
        fillColor = Gdiplus::Color(255, 38, 38, 44);
        borderColor = Gdiplus::Color(255, 50, 50, 58);
        textColor = Gdiplus::Color(255, 110, 112, 120);
    } else if (primary) {
        if (state == ButtonState::Pressed) {
            fillColor = Gdiplus::Color(255, 44, 160, 142);
            borderColor = Gdiplus::Color(255, 44, 160, 142);
        } else if (state == ButtonState::Hovered) {
            fillColor = Gdiplus::Color(255, 72, 204, 182);
            borderColor = Gdiplus::Color(255, 72, 204, 182);
        } else {
            fillColor = Gdiplus::Color(255, 56, 189, 168);
            borderColor = Gdiplus::Color(255, 56, 189, 168);
        }
        textColor = Gdiplus::Color(255, 18, 22, 24);
    } else {
        if (state == ButtonState::Pressed) {
            fillColor = Gdiplus::Color(255, 40, 40, 46);
            borderColor = Gdiplus::Color(255, 58, 58, 66);
        } else if (state == ButtonState::Hovered) {
            fillColor = Gdiplus::Color(255, 54, 54, 60);
            borderColor = Gdiplus::Color(255, 68, 68, 76);
        } else {
            fillColor = Gdiplus::Color(255, 44, 44, 50);
            borderColor = Gdiplus::Color(255, 58, 58, 66);
        }
        textColor = Gdiplus::Color(255, 232, 232, 236);
    }

    Gdiplus::SolidBrush fillBrush(fillColor);
    graphics.FillPath(&fillBrush, &bodyPath);

    Gdiplus::Pen borderPen(borderColor, 1.0f);
    graphics.DrawPath(&borderPen, &bodyPath);

    if (font && text && text[0]) {
        Gdiplus::Font gdiFont(hdc, font);
        Gdiplus::SolidBrush textBrush(textColor);
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        graphics.DrawString(text, -1, &gdiFont, bodyRect, &format, &textBrush);
    }
}

void PaintFlatCheckbox(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, const bool checked,
                       const bool enabled) {
    if (!hdc || rc.right <= rc.left || rc.bottom <= rc.top) {
        return;
    }

    constexpr float kBoxSize = 16.0f;
    constexpr float kTextGap = 10.0f;

    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);

    const Gdiplus::RectF fullRect(static_cast<Gdiplus::REAL>(rc.left), static_cast<Gdiplus::REAL>(rc.top),
                                  static_cast<Gdiplus::REAL>(rc.right - rc.left),
                                  static_cast<Gdiplus::REAL>(rc.bottom - rc.top));
    const Gdiplus::Color windowColor(255, GetRValue(g_palette.window), GetGValue(g_palette.window),
                                     GetBValue(g_palette.window));
    Gdiplus::SolidBrush windowBrush(windowColor);
    graphics.FillRectangle(&windowBrush, fullRect);

    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);

    const float boxTop = static_cast<float>(rc.top) + (fullRect.Height - kBoxSize) * 0.5f;
    const Gdiplus::RectF boxRect(static_cast<Gdiplus::REAL>(rc.left), boxTop, kBoxSize, kBoxSize);

    Gdiplus::GraphicsPath boxPath;
    AddRoundedRect(boxPath, boxRect, 3.0f);

    Gdiplus::Color fillColor;
    Gdiplus::Color borderColor;
    Gdiplus::Color textColor;
    if (!enabled) {
        fillColor = checked ? Gdiplus::Color(255, 52, 56, 62) : Gdiplus::Color(255, 38, 38, 44);
        borderColor = Gdiplus::Color(255, 58, 58, 66);
        textColor = Gdiplus::Color(255, 110, 112, 120);
    } else if (checked) {
        fillColor = Gdiplus::Color(255, 56, 189, 168);
        borderColor = Gdiplus::Color(255, 56, 189, 168);
        textColor = Gdiplus::Color(255, 232, 232, 236);
    } else {
        fillColor = Gdiplus::Color(255, 44, 44, 50);
        borderColor = Gdiplus::Color(255, 58, 58, 66);
        textColor = Gdiplus::Color(255, 232, 232, 236);
    }

    Gdiplus::SolidBrush fillBrush(fillColor);
    graphics.FillPath(&fillBrush, &boxPath);
    Gdiplus::Pen borderPen(borderColor, 1.0f);
    graphics.DrawPath(&borderPen, &boxPath);

    if (checked) {
        const Gdiplus::Color checkColor =
            enabled ? Gdiplus::Color(255, 18, 22, 24) : Gdiplus::Color(255, 150, 154, 164);
        Gdiplus::Pen checkPen(checkColor, 2.0f);
        checkPen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
        const float x = boxRect.X;
        const float y = boxRect.Y;
        graphics.DrawLine(&checkPen, x + 3.5f, y + 8.0f, x + 6.5f, y + 11.0f);
        graphics.DrawLine(&checkPen, x + 6.5f, y + 11.0f, x + 12.5f, y + 5.0f);
    }

    if (font && text && text[0]) {
        Gdiplus::Font gdiFont(hdc, font);
        Gdiplus::SolidBrush textBrush(textColor);
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentNear);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        const Gdiplus::RectF textRect(boxRect.GetRight() + kTextGap, static_cast<Gdiplus::REAL>(rc.top),
                                      static_cast<Gdiplus::REAL>(rc.right) - (boxRect.GetRight() + kTextGap),
                                      static_cast<Gdiplus::REAL>(rc.bottom - rc.top));
        graphics.DrawString(text, -1, &gdiFont, textRect, &format, &textBrush);
    }
}

}  // namespace medicat::theme
