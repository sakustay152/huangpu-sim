#include "player/TradeSystem.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "core/DomainEvents.h"
#include "core/EventBus.h"
#include "economy/Market.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "story/StoryFlags.h"

TradeResult TradeSystem::buy(GameContext& ctx, const std::string& goodsId, int qty) const {
    const GoodsDef* g = cfg_.goods(goodsId);
    if (!g) return {false, "没有这种商品: " + goodsId};
    if (qty <= 0) return {false, "数量必须为正"};

    // 地点校验：这里有没有卖
    const LocationDef* loc = cfg_.location(ctx.locationId);
    bool soldHere = std::find(g->soldAt.begin(), g->soldAt.end(), ctx.locationId) != g->soldAt.end();
    if (!soldHere) return {false, std::string("「") + g->name + "」在" + (loc ? loc->name : ctx.locationId) + "没有卖"};

    // 限购校验（每地点每日，1 行动点=1 天，分工 3.4）
    int quota = ctx.market->quotaLeft(ctx.locationId, goodsId);
    if (quota <= 0) return {false, std::string("「") + g->name + "」今天在这里的限购额度已用完，明天再来"};
    int realQty = std::min(qty, quota);

    // 单价（社交线折扣：学长资料 9 折）
    int unit = ctx.market->buyPrice(ctx.locationId, goodsId);
    if (goodsId == "kaoyan_notes" && ctx.flags->has("senior_notes"))
        unit = std::max(1, static_cast<int>(unit * cfg_.balance().kaoyanDiscount));

    // 容量校验（背包是硬约束）
    if (!ctx.player->inventory().canFit(realQty, g->slots)) {
        int fit = ctx.player->inventory().freeSlots() / g->slots;
        realQty = std::min(realQty, fit);
        if (realQty <= 0) return {false, "背包塞不下了，先卖点东西吧"};
    }

    int total = unit * realQty;
    if (!ctx.player->wallet().canAfford(total)) {
        int afford = unit > 0 ? static_cast<int>(ctx.player->wallet().get() / unit) : 0;
        realQty = std::min(realQty, afford);
        if (realQty <= 0) return {false, "现金不够，先打工或者卖货吧"};
        total = unit * realQty;
    }

    // 全部通过：落地（钱包/背包/限购/事件总线）
    ctx.player->wallet().spend(total);
    ctx.player->inventory().add(goodsId, realQty, static_cast<double>(unit), g->slots);
    ctx.market->consumeQuota(ctx.locationId, goodsId, realQty);
    ctx.bus.publish(TradeCompletedEvent{goodsId, realQty, total, true});

    return {true, "买入「" + g->name + "」x" + std::to_string(realQty) + "，支出 ¥" + std::to_string(total), total};
}

TradeResult TradeSystem::sell(GameContext& ctx, const std::string& goodsId, int qty) const {
    const GoodsDef* g = cfg_.goods(goodsId);
    if (!g) return {false, "没有这种商品: " + goodsId};
    if (qty <= 0) return {false, "数量必须为正"};

    const Inventory::Item* item = ctx.player->inventory().find(goodsId);
    if (!item) return {false, std::string("背包里没有「") + g->name + "」"};
    int realQty = std::min(qty, item->qty);

    int unit = ctx.market->sellPrice(ctx.locationId, goodsId);
    // 社交线加成：学姐内部票（演出门票卖出 +25%）
    if (goodsId == "show_ticket" && ctx.flags->has("inner_ticket"))
        unit = std::max(1, static_cast<int>(unit * cfg_.balance().innerTicketBonus));

    int total = unit * realQty;
    double costBasis = item->avgCost * realQty;
    ctx.player->wallet().add(total);
    ctx.player->inventory().remove(goodsId, realQty);
    ctx.bus.publish(TradeCompletedEvent{goodsId, realQty, total, false});

    long long profit = static_cast<long long>(total) - static_cast<long long>(costBasis);
    std::string msg = "卖出「" + g->name + "」x" + std::to_string(realQty) + "，收入 ¥" + std::to_string(total);
    if (profit >= 0) msg += "（盈利 ¥" + std::to_string(profit) + "）";
    else msg += "（亏损 ¥" + std::to_string(-profit) + "）";
    return {true, msg, total};
}

int TradeSystem::estimatePortfolioValue(const GameContext& ctx) const {
    int value = 0;
    for (const auto& item : ctx.player->inventory().items()) {
        int best = 0;
        for (const auto& loc : cfg_.locations()) {
            int p = ctx.market->sellPrice(loc.id, item.goodsId);
            if (p > best) best = p;
        }
        value += best * item.qty;
    }
    return value;
}
