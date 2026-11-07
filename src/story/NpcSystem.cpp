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

int NpcSystem::maxAffinity() const {
    int best = 0;
    for (const auto& [id, v] : affinity_) best = std::max(best, v);
    return best;
}

Json NpcSystem::toJson() const {
    Json obj = Json::object();
    for (const auto& [id, v] : affinity_) obj[id] = v;
    return Json{{"affinity", obj}};
}

void NpcSystem::fromJson(const Json& j) {
    if (!j.contains("affinity") || !j["affinity"].is_object()) return;
    for (auto it = j["affinity"].begin(); it != j["affinity"].end(); ++it) {
        if (affinity_.count(it.key())) affinity_[it.key()] = it.value().get<int>();
    }
}
