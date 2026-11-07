#pragma once
// ============================================================
// [story/NpcSystem] NPC 与好感（组员 D 主责；A 集成最小版）
// 分工 3.5：好感门槛 30/60/80 解锁渠道（情报/折扣/内部票）。
// 好感增减只能走 addAffinity（clamp 0~100）。
// ============================================================
#include <map>
#include <string>

#include "core/ISaveable.h"

class ConfigManager;

class NpcSystem : public ISaveable {
public:
    explicit NpcSystem(const ConfigManager& cfg);

    int affinity(const std::string& npcId) const;
    void addAffinity(const std::string& npcId, int delta);
    bool atLeast(const std::string& npcId, int v) const { return affinity(npcId) >= v; }

    // 全 NPC 里最高的好感（结局判定用）
    int maxAffinity() const;

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    const ConfigManager& cfg_;
    std::map<std::string, int> affinity_;
};
