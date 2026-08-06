#pragma once

#include <string>
#include <vector>

namespace medicat::i18n {

// Detect UI language from Windows (en, es, fr, pl, tr, or fallback en).
std::wstring DetectLanguage();

// Active UI language code (e.g. en, es) set by Load().
std::wstring CurrentLanguage();

bool Load(const std::wstring& languageCode = L"");

// Lookup "category.key" e.g. Tr(L"ui.install_button")
std::wstring Get(const std::wstring& key);

// English-only lookup for telemetry/support uploads.
std::wstring GetEnglish(const std::wstring& key);

// Best-effort reverse lookup: localized UI string -> English equivalent.
std::wstring ToEnglish(const std::wstring& localizedText);

// Replace {0}, {1}, ... placeholders.
std::wstring Format(const std::wstring& text, const std::vector<std::wstring>& args = {});

inline std::wstring Tr(const std::wstring& key) { return Get(key); }

inline std::wstring Tr(const std::wstring& key, const std::wstring& a0) {
    return Format(Get(key), {a0});
}

inline std::wstring Tr(const std::wstring& key, const std::wstring& a0, const std::wstring& a1) {
    return Format(Get(key), {a0, a1});
}

inline std::wstring Tr(const std::wstring& key, const std::wstring& a0, const std::wstring& a1,
                       const std::wstring& a2) {
    return Format(Get(key), {a0, a1, a2});
}

inline std::wstring Tr(const std::wstring& key, const std::wstring& a0, const std::wstring& a1,
                       const std::wstring& a2, const std::wstring& a3) {
    return Format(Get(key), {a0, a1, a2, a3});
}

}  // namespace medicat::i18n
