#include "util/StringUtils.h"

namespace strutil {

std::size_t displayWidth(const std::string& s) {
    std::size_t w = 0;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char b = static_cast<unsigned char>(s[i]);
        if (b < 0x80) { w += 1; i += 1; }
        else if ((b >> 5) == 0x6) { w += 2; i += 2; }       // 2 字节
        else if ((b >> 4) == 0xE) { w += 2; i += 3; }       // 3 字节（常用汉字）
        else if ((b >> 3) == 0x1E) { w += 2; i += 4; }      // 4 字节
        else { w += 1; i += 1; }
    }
    return w;
}

std::string padRight(const std::string& s, std::size_t width) {
    std::size_t w = displayWidth(s);
    if (w >= width) return s;
    return s + std::string(width - w, ' ');
}

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        std::size_t j = i;
        while (j < s.size() && !std::isspace(static_cast<unsigned char>(s[j]))) ++j;
        if (j > i) out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

} // namespace strutil
