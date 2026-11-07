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

} // namespace console
