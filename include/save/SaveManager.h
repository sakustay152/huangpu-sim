#pragma once
// ============================================================
// [save/SaveManager] 存档管理（组长 A 维护）
// 分工 7.2：3 个手动槽位 + 自动存档（每周日）；JSON；版本号；
// 原子写入（先写 .tmp 再替换）；坏档返回错误信息不崩溃。
// ============================================================
#include <string>
#include <vector>

#include "util/Json.h"

class SaveManager {
public:
    static constexpr int kAutoSlot = 0;
    static constexpr int kManualSlots = 3; // 1..3

    explicit SaveManager(std::string dir) : dir_(std::move(dir)) {}

    struct SlotInfo {
        int slot = 0;
        bool exists = false;
        int version = 0;
        std::string summary; // 给玩家看的一行摘要
    };

    bool save(int slot, const Json& data, std::string* err = nullptr);
    Json load(int slot, std::string* err = nullptr); // 失败返回 null 且带 err
    std::vector<SlotInfo> listSlots() const;

private:
    std::string pathFor(int slot) const;
    void ensureDir() const;

    std::string dir_;
};
