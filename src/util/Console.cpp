#include "util/Console.h"

#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace console {

const char* const RESET   = "\x1b[0m";
const char* const RED     = "\x1b[31m";
const char* const GREEN   = "\x1b[32m";
const char* const YELLOW  = "\x1b[33m";
const char* const CYAN    = "\x1b[36m";
const char* const MAGENTA = "\x1b[35m";
const char* const DIM     = "\x1b[2m";
const char* const BOLD    = "\x1b[1m";

void init() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);      // 输出 UTF-8（源码与数据文件均为 UTF-8）
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING); // 启用 ANSI 颜色
    }
#endif
}

void clearScreen() {
    std::cout << "\x1b[2J\x1b[H";
}

std::string paint(const std::string& s, const char* color) {
    return std::string(color) + s + RESET;
}

} // namespace console
