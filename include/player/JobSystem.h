#pragma once
// ============================================================
// [player/JobSystem] 打工（组员 C）
// 分工 7.4：3 种岗位，收益/体力不同；学长好感 ≥30 日薪 +20%。
// 规则：体力不足不禁止打工，只按 lowEnergyPayRate 减收益（避免 Bot 卡死）。
// ============================================================
#include <string>

class ConfigManager;
class GameContext;

struct JobResult {
    bool ok = true;
    std::string jobName;
    int pay = 0;
    int staminaCost = 0;
    bool lowEnergy = false;
    std::string msg;
};

class JobSystem {
public:
    explicit JobSystem(const ConfigManager& cfg) : cfg_(cfg) {}
    JobResult work(GameContext& ctx) const;

private:
    const ConfigManager& cfg_;
};
