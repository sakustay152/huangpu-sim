#pragma once
// ============================================================
// [player/Energy] 体力系统（组员 C 主责的 EnergySystem 轻量回归版）
// 分工 3.2 的"可砍项"在炒股功能上线后回归：盯盘消耗体力，
// 休息恢复，每周结算小幅回血；低体力时炒股更容易被割。
// ============================================================
#include "core/ISaveable.h"
#include "util/Json.h"

class ConfigManager;

class EnergySystem : public ISaveable {
public:
    explicit EnergySystem(const ConfigManager& cfg);

    int get() const { return stamina_; }
    int max() const { return max_; }
    bool isLow() const;    // 低于阈值（炒股失败概率上升）

    // 消耗体力：不足返回 false 且不扣
    bool spend(int cost);
    void add(int amount);  // 恢复，clamp 到 [0, max]

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    int max_;
    int lowThreshold_;
    int stamina_;
};
