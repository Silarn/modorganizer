// Some string utilities.
#pragma once
#include <algorithm>
#include <cctype>
#include <codecvt>
#include <locale>
#include <string>
#include <vector>

namespace common {

// Splits the string `str` by `delim` into `lines`
// Taken from https://stackoverflow.com/a/9676623/3665377
static inline void split(std::vector<std::string>& lines, const std::string& str, const std::string& delim) {
    std::string::size_type start = 0;
    auto pos = str.find(delim, start);
    while (pos != std::string::npos) {
        if (pos != start) { // ignore empty tokens
            lines.emplace_back(str, start, pos - start);
        }
        start = pos + 1;
        pos = str.find(delim, start);
    }
    // ignore trailing delimiter
    if (start < str.length()) {
        lines.emplace_back(str, start, str.length() - start); // add what's left of the string
    }
}

// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

// Check if basic_string<> starts with something.
template <typename T>
static inline bool starts_with(const T& big_str, const T& small_str) {
    return big_str.compare(0, small_str.length(), small_str) == 0;
}

static inline std::wstring toWString(const std::string& str) {
    using convert_typeX = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

static inline std::string toString(const std::wstring& wstr) {
    using convert_typeX = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

static inline std::string toLower(std::string s) {
    // TODO: https://stackoverflow.com/a/24063783/3665377 and proper comparision.
    // This will fail on unicode.
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Two string case isentive equal
static inline bool iequals(std::string a, std::string b) { return toLower(a) == toLower(b); }

} // namespace common
