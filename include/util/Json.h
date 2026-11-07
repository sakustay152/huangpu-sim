#pragma once
// ============================================================
// [util/Json] 统一 JSON 入口（组长 A 维护）
// 全项目通过本头文件使用 nlohmann/json（已内置于 third_party/，
// 离线可编译，无需 vcpkg）。若日后换 JSON 库，只改这里。
// ============================================================
#include <nlohmann/json.hpp>

using Json = nlohmann::json;
