#pragma once
// ============================================================
// [util/Console] 控制台初始化与颜色（组长 A 维护）
// Windows: 设 UTF-8 代码页 + 开启 ANSI 转义；其它平台直接 ANSI。
// E 的 SFML UI 接管后，本文件与 ui/ 控制台实现一起退役。
// ============================================================
#include <string>

namespace console {

// 初始化（UTF-8 + VT100），进程内调用一次即可
void init();

void clearScreen();

// 颜色常量（ANSI 转义序列）
extern const char* const RESET;
extern const char* const RED;
extern const char* const GREEN;
extern const char* const YELLOW;
extern const char* const CYAN;
extern const char* const MAGENTA;
extern const char* const DIM;
extern const char* const BOLD;

// 给字符串包一层颜色，返回带转义的字符串
std::string paint(const std::string& s, const char* color);

// ---- 交互模式（炒股屏用）：鼠标点击 + 无回显按键 ----
struct ConsoleEvent {
    bool has = false;
    bool mouse = false;   // true = 鼠标事件；false = 键盘
    bool clicked = false; // 左键单击
    int x = 0;            // 鼠标所在字符列
    int y = 0;            // 鼠标所在字符行
    char key = 0;         // 键盘字符（小写化；ESC = 27）
};

// 标准输入是否为真实控制台（重定向/管道时炒股屏回退到行输入模式）
bool stdinIsConsole();

// RAII：进入交互模式（启用鼠标、关闭 QuickEdit、隐藏光标），析构时全部恢复
class InteractiveGuard {
public:
    InteractiveGuard();
    ~InteractiveGuard();
    InteractiveGuard(const InteractiveGuard&) = delete;
    InteractiveGuard& operator=(const InteractiveGuard&) = delete;

private:
    void* hIn_ = nullptr;
    unsigned long oldIn_ = 0;
    void* hOut_ = nullptr;
    unsigned long oldCursorSize_ = 0;
    unsigned long oldCursorVisible_ = 0;
};

// 阻塞等待一次输入事件（鼠标左键 / 可打印按键 / ESC），timeoutMs 内无事件返回 false
bool pollEvent(int timeoutMs, ConsoleEvent& ev);

} // namespace console
