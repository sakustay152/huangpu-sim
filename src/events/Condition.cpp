#include "events/Condition.h"

#include <stdexcept>

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"
#include "util/JsonUtil.h"

bool WeekBetween::test(const GameContext& ctx) const {
    int w = ctx.time->week();
    return w >= min_ && w <= max_;
}

bool LocationIs::test(const GameContext& ctx) const {
    return ctx.locationId == locId_;
}

bool MinMoney::test(const GameContext& ctx) const {
    return ctx.player->wallet().get() >= v_;
}

bool TotalAssetsMin::test(const GameContext& ctx) const {
    return ctx.player->wallet().get() + ctx.trade->estimatePortfolioValue(ctx) >= v_;
}

bool HasFlag::test(const GameContext& ctx) const {
    // "not:xxx" 前缀 = 取反（结局条件里用：not:failed 表示"没挂科"）
    if (flag_.size() > 4 && flag_.compare(0, 4, "not:") == 0)
        return !ctx.flags->has(flag_.substr(4));
    return ctx.flags->has(flag_);
}

bool AffinityMin::test(const GameContext& ctx) const {
    return ctx.npcs->atLeast(npcId_, v_);
}

bool AffinityAnyMin::test(const GameContext& ctx) const {
    return ctx.npcs->maxAffinity() >= v_;
}

bool InventoryNotEmpty::test(const GameContext& ctx) const {
    return !ctx.player->inventory().empty();
}

std::unique_ptr<ICondition> ConditionFactory::create(const Json& j) {
    std::string type = ju::getOr<std::string>(j, "type", "");
    if (type == "weekBetween")
        return std::make_unique<WeekBetween>(ju::getOr(j, "min", 1), ju::getOr(j, "max", 12));
    if (type == "locationIs")
        return std::make_unique<LocationIs>(ju::getOr<std::string>(j, "location", ""));
    if (type == "minMoney")
        return std::make_unique<MinMoney>(ju::getOr(j, "value", 0));
    if (type == "totalAssetsMin")
        return std::make_unique<TotalAssetsMin>(ju::getOr(j, "value", 0));
    if (type == "hasFlag")
        return std::make_unique<HasFlag>(ju::getOr<std::string>(j, "flag", ""));
    if (type == "affinityMin")
        return std::make_unique<AffinityMin>(ju::getOr<std::string>(j, "npc", ""),
                                             ju::getOr(j, "value", 0));
    if (type == "affinityAnyMin")
        return std::make_unique<AffinityAnyMin>(ju::getOr(j, "value", 0));
    if (type == "inventoryNotEmpty")
        return std::make_unique<InventoryNotEmpty>();
    if (type == "always")
        return std::make_unique<AlwaysTrue>();
    throw std::runtime_error("未知条件类型: \"" + type + "\"（events/endings.json 校验）");
}
