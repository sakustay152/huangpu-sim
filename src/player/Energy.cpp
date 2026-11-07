#include "player/Energy.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "util/JsonUtil.h"

EnergySystem::EnergySystem(const ConfigManager& cfg)
    : max_(cfg.balance().staminaMax),
      lowThreshold_(cfg.balance().staminaLowThreshold),
      stamina_(cfg.balance().staminaMax) {}

bool EnergySystem::isLow() const {
    return stamina_ < lowThreshold_;
}

bool EnergySystem::spend(int cost) {
    if (cost < 0 || stamina_ < cost) return false;
    stamina_ -= cost;
    return true;
}

void EnergySystem::add(int amount) {
    if (amount <= 0) return;
    stamina_ = std::min(max_, stamina_ + amount);
}

Json EnergySystem::toJson() const {
    return Json{{"stamina", stamina_}, {"max", max_}};
}

void EnergySystem::fromJson(const Json& j) {
    stamina_ = ju::getOr(j, "stamina", stamina_);
    stamina_ = std::clamp(stamina_, 0, max_);
}
