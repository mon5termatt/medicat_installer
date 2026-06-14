#include "i18n.h"

#include "i18n_generated.h"

#include <windows.h>

#include <unordered_map>

namespace medicat::i18n {

namespace {

std::wstring g_language = L"en";
std::unordered_map<std::wstring, std::wstring> g_strings;

void IndexLanguage(const std::wstring& lang) {
    g_strings.clear();
    for (const auto& entry : generated::StringsFor(lang)) {
        g_strings.emplace(entry.key, entry.value);
    }
}

std::wstring LangFromLcid(LCID lcid) {
    wchar_t code[16]{};
    if (GetLocaleInfoW(lcid, LOCALE_SISO639LANGNAME, code, 16) == 0) {
        return L"en";
    }
    const std::wstring lang = code;
    for (size_t i = 0; i < generated::SupportedLanguageCount; ++i) {
        if (lang == generated::SupportedLanguages[i]) {
            return lang;
        }
    }
    return L"en";
}

}  // namespace

std::wstring DetectLanguage() {
    return LangFromLcid(GetUserDefaultUILanguage());
}

bool Load(const std::wstring& languageCode) {
    std::wstring lang = languageCode.empty() ? DetectLanguage() : languageCode;
    IndexLanguage(lang);
    if (g_strings.empty()) {
        lang = L"en";
        IndexLanguage(lang);
    }
    g_language = lang;
    return !g_strings.empty();
}

std::wstring Get(const std::wstring& key) {
    const auto it = g_strings.find(key);
    if (it != g_strings.end()) {
        return it->second;
    }
    return key;
}

std::wstring Format(const std::wstring& text, const std::vector<std::wstring>& args) {
    std::wstring out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'{' && i + 2 < text.size() && text[i + 2] == L'}' &&
            text[i + 1] >= L'0' && text[i + 1] <= L'9') {
            const size_t index = static_cast<size_t>(text[i + 1] - L'0');
            if (index < args.size()) {
                out += args[index];
            }
            i += 2;
            continue;
        }
        out.push_back(text[i]);
    }
    return out;
}

}  // namespace medicat::i18n
