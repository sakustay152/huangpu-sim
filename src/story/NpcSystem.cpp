#include "story/NpcSystem.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "util/JsonUtil.h"

NpcSystem::NpcSystem(const ConfigManager& cfg) : cfg_(cfg) {
    for (const auto& n : cfg.npcs()) affinity_[n.id] = 0;
}

int NpcSystem::affinity(const std::string& npcId) const {
    auto it = affinity_.find(npcId);
    return it == affinity_.end() ? 0 : it->second;
}

void NpcSystem::addAffinity(const std::string& npcId, int delta) {
    int& v = affinity_[npcId];
    v += delta;
    if (v < 0) v = 0;
    if (v > 100) v = 100;
}

void NpcSystem::markSocial(const std::string& npcId, int week) {
    lastSocialWeek_[npcId] = week;
}

void NpcSystem::applyWeeklyDecay(int decay, int settledWeek) {
    if (decay <= 0) return;
    for (auto& [id, v] : affinity_) {
        auto it = lastSocialWeek_.find(id);
        // 本周（settledWeek）没与该 NPC 社交过 → 好感自然衰减
        if (it == lastSocialWeek_.end() || it->second < settledWeek) {
            v = std::max(0, v - decay);
        }
    }
}

int NpcSystem::maxAffinity() const {
    int best = 0;
    for (const auto& [id, v] : affinity_) best = std::max(best, v);
    return best;
}

Json NpcSystem::toJson() const {
    Json obj = Json::object();
    for (const auto& [id, v] : affinity_) obj[id] = v;
    Json social = Json::object();
    for (const auto& [id, w] : lastSocialWeek_) social[id] = w;
    return Json{{"affinity", obj}, {"lastSocialWeek", social}};
}

void NpcSystem::fromJson(const Json& j) {
    if (!j.contains("affinity") || !j["affinity"].is_object()) return;
    for (auto it = j["affinity"].begin(); it != j["affinity"].end(); ++it) {
        if (affinity_.count(it.key())) affinity_[it.key()] = it.value().get<int>();
    }
    lastSocialWeek_.clear();
    if (j.contains("lastSocialWeek") && j["lastSocialWeek"].is_object())
        for (auto it = j["lastSocialWeek"].begin(); it != j["lastSocialWeek"].end(); ++it)
            if (affinity_.count(it.key())) lastSocialWeek_[it.key()] = it.value().get<int>();
}
