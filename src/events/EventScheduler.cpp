#include "events/EventScheduler.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "events/Effect.h"
#include "events/EventFactory.h"
#include "events/GameEvent.h"
#include "time/TimeSystem.h"
#include "util/JsonUtil.h"

EventScheduler::EventScheduler(const ConfigManager& cfg, Random& rng) : cfg_(cfg), rng_(rng) {}

void EventScheduler::buildFromConfig() {
    all_ = EventFactory::buildAll(cfg_);
    cooldown_.clear();
    scheduled_.clear();
}

std::vector<const GameEvent*> EventScheduler::fixedForWeek(int week, const GameContext& ctx) const {
    std::vector<const GameEvent*> out;
    for (const auto& ev : all_) {
        auto* f = dynamic_cast<const FixedEvent*>(ev.get());
        if (f && f->week == week && ev->canTrigger(ctx)) out.push_back(ev.get());
    }
    return out;
}

const GameEvent* EventScheduler::rollRandom(const GameContext& ctx) {
    // 候选：随机事件 + 条件通过 + 冷却结束
    std::vector<const RandomEvent*> candidates;
    std::vector<int> weights;
    for (const auto& ev : all_) {
        auto* r = dynamic_cast<const RandomEvent*>(ev.get());
        if (!r) continue;
        auto it = cooldown_.find(r->id);
        if (it != cooldown_.end() && it->second > 0) continue;
        if (!r->canTrigger(ctx)) continue;
        candidates.push_back(r);
        weights.push_back(std::max(1, r->weight(ctx)));
    }
    if (candidates.empty()) return nullptr;
    int total = 0;
    for (int w : weights) total += w;
    int roll = rng_.range(1, total);
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        roll -= weights[i];
        if (roll <= 0) return candidates[i];
    }
    return candidates.back();
}

void EventScheduler::markTriggered(const GameEvent& ev) {
    auto* r = dynamic_cast<const RandomEvent*>(&ev);
    if (r && r->cooldownWeeks > 0) cooldown_[r->id] = r->cooldownWeeks + 1; // 本周结算时递减一次
}

void EventScheduler::scheduleEffect(int dueWeek, Json effectJson) {
    scheduled_.emplace_back(dueWeek, std::move(effectJson));
}

void EventScheduler::applyDueEffects(GameContext& ctx) {
    // 周结算发生在周翻转之后：正在结算的是 time->settledWeek()
    int now = ctx.time->settledWeek();
    for (auto it = scheduled_.begin(); it != scheduled_.end();) {
        if (it->first <= now) {
            auto eff = EffectFactory::create(it->second);
            eff->apply(ctx);
            it = scheduled_.erase(it);
        } else {
            ++it;
        }
    }
}

void EventScheduler::onWeekStart() {
    for (auto it = cooldown_.begin(); it != cooldown_.end();) {
        if (--it->second <= 0) it = cooldown_.erase(it);
        else ++it;
    }
}

Json EventScheduler::toJson() const {
    Json cd = Json::object();
    for (const auto& [id, w] : cooldown_) cd[id] = w;
    Json sched = Json::array();
    for (const auto& [week, eff] : scheduled_)
        sched.push_back(Json{{"week", week}, {"effect", eff}});
    return Json{{"cooldown", cd}, {"scheduled", sched}};
}

void EventScheduler::fromJson(const Json& j) {
    cooldown_.clear();
    if (j.contains("cooldown") && j["cooldown"].is_object())
        for (auto it = j["cooldown"].begin(); it != j["cooldown"].end(); ++it)
            cooldown_[it.key()] = it.value().get<int>();
    scheduled_.clear();
    if (j.contains("scheduled") && j["scheduled"].is_array())
        for (const auto& e : j["scheduled"])
            scheduled_.emplace_back(ju::getOr(e, "week", 1), e.value("effect", Json::object()));
}
