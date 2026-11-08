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

// ---------------- 交互模式（鼠标 + 原始按键） ----------------

bool stdinIsConsole() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    return h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode) != 0;
#else
    return false;
#endif
}

InteractiveGuard::InteractiveGuard() {
#ifdef _WIN32
    hIn_ = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (hIn_ != INVALID_HANDLE_VALUE && GetConsoleMode(hIn_, &mode)) {
        oldIn_ = mode;
        DWORD m = mode;
        m |= ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
        m &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
               ENABLE_PROCESSED_INPUT);
        SetConsoleMode(hIn_, m);
    }
    hOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cur{};
    if (hOut_ != INVALID_HANDLE_VALUE && GetConsoleCursorInfo(hOut_, &cur)) {
        oldCursorSize_ = cur.dwSize;
        oldCursorVisible_ = cur.bVisible ? 1 : 0;
        cur.bVisible = FALSE;
        SetConsoleCursorInfo(hOut_, &cur);
    }
#endif
}

InteractiveGuard::~InteractiveGuard() {
#ifdef _WIN32
    if (hIn_ != INVALID_HANDLE_VALUE && hIn_ != nullptr) SetConsoleMode(hIn_, oldIn_);
    if (hOut_ != INVALID_HANDLE_VALUE && hOut_ != nullptr) {
        CONSOLE_CURSOR_INFO cur{};
        cur.dwSize = static_cast<DWORD>(oldCursorSize_);
        cur.bVisible = oldCursorVisible_ != 0;
        SetConsoleCursorInfo(hOut_, &cur);
    }
#endif
}

bool pollEvent(int timeoutMs, ConsoleEvent& ev) {
#ifdef _WIN32
    ev = ConsoleEvent{};
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return false;
    if (WaitForSingleObject(h, static_cast<DWORD>(timeoutMs)) != WAIT_OBJECT_0) return false;
    INPUT_RECORD rec[16];
    DWORD n = 0;
    if (!ReadConsoleInput(h, rec, 16, &n)) return false;
    for (DWORD i = 0; i < n; ++i) {
        const INPUT_RECORD& r = rec[i];
        if (r.EventType == MOUSE_EVENT &&
            (r.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) &&
            r.Event.MouseEvent.dwEventFlags == 0) {
            ev.has = true;
            ev.mouse = true;
            ev.clicked = true;
            ev.x = static_cast<int>(r.Event.MouseEvent.dwMousePosition.X);
            ev.y = static_cast<int>(r.Event.MouseEvent.dwMousePosition.Y);
            return true;
        }
        if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown) {
            CHAR c = r.Event.KeyEvent.uChar.AsciiChar;
            if (c == 27) { // ESC
                ev.has = true;
                ev.key = 27;
                return true;
            }
            if (c >= '0' && c <= '9') { ev.has = true; ev.key = c; return true; }
            if (c >= 'a' && c <= 'z') { ev.has = true; ev.key = c; return true; }
            if (c >= 'A' && c <= 'Z') { ev.has = true; ev.key = c - 'A' + 'a'; return true; }
        }
    }
    return false;
#else
    (void)timeoutMs;
    ev = ConsoleEvent{};
    return false;
#endif
}

} // namespace console
