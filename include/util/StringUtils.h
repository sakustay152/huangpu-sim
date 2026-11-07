#pragma once
// ============================================================
// [util/StringUtils] 字符串工具（组长 A 维护）
// 重点：中文按 2 列宽计算，用于控制台表格对齐（手账风 UI 的排版基础）
// ============================================================
#include <string>
#include <vector>

namespace strutil {

// UTF-8 字符串的终端显示宽度：ASCII 记 1，中日韩等全角记 2
std::size_t displayWidth(const std::string& s);

// 右侧补空格到指定显示宽度（超出则原样返回）
std::string padRight(const std::string& s, std::size_t width);

// 按任意空白切分
std::vector<std::string> split(const std::string& s);

// 去首尾空白
std::string trim(const std::string& s);

// 转小写（仅 ASCII）
std::string toLower(std::string s);

} // namespace strutil
