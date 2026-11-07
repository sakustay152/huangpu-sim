#pragma once
// ============================================================
// [util/JsonUtil] JSON 读写工具（组长 A 维护）
// - getOr: 带默认值的字段读取（配置/存档缺失字段不崩溃）
// - loadFile: 读取并解析 JSON，失败时抛出带文件名的异常（配置错误是致命错误，
//              游戏运行期逻辑一律用返回值/错误码，不用异常做流程控制）
// ============================================================
#include <string>
#include "util/Json.h"

namespace ju {

// 读取 j[key]，缺失或类型不符时返回 def
template <typename T>
T getOr(const Json& j, const char* key, T def) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return def;
    try {
        return it->get<T>();
    } catch (...) {
        return def;
    }
}

// 从文件加载 JSON；解析失败抛 std::runtime_error（含文件路径与原因）
Json loadFile(const std::string& path);

// 写 JSON 到文件（UTF-8，缩进 2 空格）
void saveFile(const std::string& path, const Json& j);

} // namespace ju
