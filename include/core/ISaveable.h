#pragma once
// ============================================================
// [core/ISaveable] 存档接口（组长 A 维护，W1 冻结接口）
// 分工 8.1：所有需要进存档的系统实现本接口；
// SaveManager 只认 ISaveable，不关心具体是哪个系统。
// ============================================================
#include "util/Json.h"

class ISaveable {
public:
    virtual ~ISaveable() = default;

    // 序列化本系统的全部状态
    virtual Json toJson() const = 0;

    // 从存档恢复状态（只恢复自身，不碰别的系统）
    virtual void fromJson(const Json& j) = 0;
};
