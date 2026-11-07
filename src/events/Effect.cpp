#include "events/Effect.h"

#include <algorithm>
#include <stdexcept>

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "economy/Market.h"
#include "events/Condition.h"
#include "events/EventScheduler.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"
#include "util/JsonUtil.h"

std::string MoneyEffect::apply(GameContext& ctx) const {
    const ConfigManager& cfg = ctx.config;
    if (amount_ >= 0) {
        ctx.player->wallet().add(amount_);
        return "现金 +" + std::to_string(amount_);
    }
    int want = -amount_;
    int take = std::min(ctx.player->wallet().get(), want);
    if (take > 0) ctx.player->wallet().spend(take);
    return take >= want ? "现金 -" + std::to_string(want)
                        : "现金 -" + std::to_string(take) + "（现金不足，掏空了钱包）";
}

std::string RandomMoneyEffect::apply(GameContext& ctx) const {
    int amount = ctx.rng.range(std::min(min_, max_), std::max(min_, max_));
    if (amount > 0) ctx.player->wallet().add(amount);
    return "现金 +" + std::to_string(amount);
}

std::string AffinityEffect::apply(GameContext& ctx) const {
    const NpcDef* n = ctx.config.npc(npcId_);
    if (!n) return "（无效的 NPC: " + npcId_ + "）";
    int before = ctx.npcs->affinity(npcId_);
    ctx.npcs->addAffinity(npcId_, delta_);
    int after = ctx.npcs->affinity(npcId_);
    std::string msg = n->name + " 好感 " + (delta_ >= 0 ? "+" : "") + std::to_string(delta_) +
                      "（" + std::to_string(before) + "→" + std::to_string(after) + "）";
    // 跨过解锁门槛时给出提示（分工 3.5 社交线回报）
    for (int gate : {30, 60, 80}) {
        if (before < gate && after >= gate) {
            msg += "｜解锁新渠道！";
            if (npcId_ == "roommate" && gate == 30) msg += "（室友每周多给一条二手货情报）";
            if (npcId_ == "senior" && gate == 30) msg += "（学长介绍打工，日薪 +20%）";
            if (npcId_ == "senior" && gate == 60) msg += "（考研资料进货价 9 折）";
            if (npcId_ == "xuejie" && gate == 30) msg += "（学姐带你进活动圈子）";
            if (npcId_ == "xuejie" && gate == 60) msg += "（演出内部票渠道开通）";
        }
    }
    return msg;
}

std::string FlagEffect::apply(GameContext& ctx) const {
    ctx.flags->set(flag_);
    return "获得剧情标记：" + flag_;
}

std::string PriceShockEffect::apply(GameContext& ctx) const {
    const GoodsDef* g = ctx.config.goods(goodsId_);
    if (!g) return "（无效的商品: " + goodsId_ + "）";
    ctx.market->applyShock(goodsId_, mult_, days_);
    return "「" + g->name + "」价格" + (mult_ >= 1.0 ? "暴涨" : "暴跌") + "（x" +
           std::to_string(mult_).substr(0, 4) + "，持续 " + std::to_string(days_) + " 天）";
}

std::string ItemEffect::apply(GameContext& ctx) const {
    const GoodsDef* g = ctx.config.goods(goodsId_);
    if (!g) return "（无效的商品: " + goodsId_ + "）";
    int fit = ctx.player->inventory().freeSlots() / g->slots;
    int realQty = std::min(qty_, fit);
    if (realQty <= 0) return "「" + g->name + "」x" + std::to_string(qty_) + " 放不下，只好放弃";
    ctx.player->inventory().add(goodsId_, realQty, 0.0, g->slots);
    std::string msg = "获得「" + g->name + "」x" + std::to_string(realQty);
    if (realQty < qty_) msg += "（背包满了，只装下一部分）";
    return msg;
}

int ItemEffect::estimateMoneyDelta() const {
    return 0; // 保守估计：免费物品不算现金收益
}

std::string StudyEffect::apply(GameContext& ctx) const {
    ctx.player->addStudy(delta_);
    if (delta_ > 0) ctx.time->markStudied();
    std::string sign = delta_ >= 0 ? "+" : "";
    return "平时分 " + sign + std::to_string(delta_) +
           "（当前 " + std::to_string(ctx.player->studyScore()) + "）";
}

std::string ScheduledEffect::apply(GameContext& ctx) const {
    int due = ctx.time->week() + weeks_;
    ctx.events->scheduleEffect(due, effectJson_);
    return "（这件事将在第 " + std::to_string(due) + " 周有结果）";
}

IfEffect::IfEffect(std::unique_ptr<ICondition> cond, std::vector<std::unique_ptr<IEffect>> effects)
    : cond_(std::move(cond)), effects_(std::move(effects)) {}

IfEffect::~IfEffect() = default;

std::string IfEffect::apply(GameContext& ctx) const {
    if (cond_ && !cond_->test(ctx)) return "";
    std::string out;
    for (const auto& e : effects_) {
        std::string m = e->apply(ctx);
        if (!m.empty()) out += (out.empty() ? "" : "；") + m;
    }
    return out;
}

int IfEffect::estimateMoneyDelta() const {
    int sum = 0;
    for (const auto& e : effects_) sum += e->estimateMoneyDelta();
    return sum;
}

std::unique_ptr<IEffect> EffectFactory::create(const Json& j) {
    std::string type = ju::getOr<std::string>(j, "type", "");
    if (type == "money")
        return std::make_unique<MoneyEffect>(ju::getOr(j, "amount", 0));
    if (type == "randomMoney")
        return std::make_unique<RandomMoneyEffect>(ju::getOr(j, "min", 0), ju::getOr(j, "max", 0));
    if (type == "affinity")
        return std::make_unique<AffinityEffect>(ju::getOr<std::string>(j, "npc", ""),
                                                ju::getOr(j, "delta", 0));
    if (type == "flag")
        return std::make_unique<FlagEffect>(ju::getOr<std::string>(j, "flag", ""));
    if (type == "priceShock")
        return std::make_unique<PriceShockEffect>(ju::getOr<std::string>(j, "goods", ""),
                                                  ju::getOr(j, "mult", 1.0),
                                                  ju::getOr(j, "days", 1));
    if (type == "item")
        return std::make_unique<ItemEffect>(ju::getOr<std::string>(j, "goods", ""),
                                            ju::getOr(j, "qty", 1));
    if (type == "study")
        return std::make_unique<StudyEffect>(ju::getOr(j, "delta", 0));
    if (type == "scheduled")
        return std::make_unique<ScheduledEffect>(ju::getOr(j, "weeks", 1), j.value("effect", Json::object()));
    if (type == "if") {
        auto cond = ConditionFactory::create(j.value("cond", Json::object()));
        std::vector<std::unique_ptr<IEffect>> effects;
        if (j.contains("effects") && j["effects"].is_array())
            for (const auto& e : j["effects"]) effects.push_back(EffectFactory::create(e));
        return std::make_unique<IfEffect>(std::move(cond), std::move(effects));
    }
    throw std::runtime_error("未知效果类型: \"" + type + "\"（events.json 校验）");
}
