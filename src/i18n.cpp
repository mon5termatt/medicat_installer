#include "i18n.h"

#include "i18n_generated.h"

#include <windows.h>

#include <unordered_map>
#include <vector>

namespace medicat::i18n {

namespace {

std::wstring g_language = L"en";
std::unordered_map<std::wstring, std::wstring> g_strings;

void IndexLanguage(const std::wstring& lang) {
    g_strings.clear();

    for (const auto& entry : generated::StringsFor(L"en")) {
        g_strings.emplace(entry.key, entry.value);
    }

    for (const auto& entry : generated::StringsFor(lang)) {
        g_strings[entry.key] = entry.value;
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

std::wstring CurrentLanguage() {
    return g_language;
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

std::wstring GetEnglish(const std::wstring& key) {
    for (const auto& entry : generated::StringsFor(L"en")) {
        if (key == entry.key) {
            return entry.value;
        }
    }
    return Get(key);
}

bool MatchI18nTemplate(const std::wstring& text, const std::wstring& pattern, std::vector<std::wstring>& args) {
    args.clear();
    size_t textPos = 0;
    size_t patPos = 0;

    while (patPos < pattern.size()) {
        const size_t brace = pattern.find(L'{', patPos);
        if (brace == std::wstring::npos) {
            return text.compare(textPos, std::wstring::npos, pattern.substr(patPos)) == 0;
        }

        const std::wstring literal = pattern.substr(patPos, brace - patPos);
        if (!literal.empty()) {
            if (text.compare(textPos, literal.size(), literal) != 0) {
                return false;
            }
            textPos += literal.size();
        }

        if (brace + 2 >= pattern.size() || pattern[brace + 2] != L'}') {
            return false;
        }

        const size_t placeholderEnd = brace + 3;
        const size_t nextBrace = pattern.find(L'{', placeholderEnd);
        const std::wstring nextLiteral =
            nextBrace == std::wstring::npos ? pattern.substr(placeholderEnd) : pattern.substr(placeholderEnd, nextBrace - placeholderEnd);

        size_t argEnd = std::wstring::npos;
        if (nextLiteral.empty()) {
            argEnd = text.size();
        } else {
            argEnd = text.find(nextLiteral, textPos);
            if (argEnd == std::wstring::npos) {
                return false;
            }
        }

        args.push_back(text.substr(textPos, argEnd - textPos));
        textPos = argEnd;
        patPos = nextLiteral.empty() ? pattern.size() : placeholderEnd;
    }

    return textPos == text.size();
}

std::wstring TranslateSingleToEnglish(const std::wstring& text) {
    if (text.empty() || g_language == L"en") {
        return text;
    }

    for (const auto& entry : generated::StringsFor(g_language)) {
        if (text == entry.value) {
            return GetEnglish(entry.key);
        }
    }

    for (const auto& entry : generated::StringsFor(g_language)) {
        std::vector<std::wstring> args;
        if (MatchI18nTemplate(text, entry.value, args)) {
            return Format(GetEnglish(entry.key), args);
        }
    }

    return text;
}

std::wstring ToEnglish(const std::wstring& localizedText) {
    if (localizedText.empty() || g_language == L"en") {
        return localizedText;
    }

    std::vector<std::wstring> parts;
    size_t start = 0;
    while (start <= localizedText.size()) {
        const size_t split = localizedText.find(L"\n\n", start);
        if (split == std::wstring::npos) {
            parts.push_back(localizedText.substr(start));
            break;
        }
        parts.push_back(localizedText.substr(start, split - start));
        start = split + 2;
    }

    std::wstring out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out += L"\n\n";
        }
        out += TranslateSingleToEnglish(parts[i]);
    }
    return out;
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
