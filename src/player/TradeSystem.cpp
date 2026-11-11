#include "player/TradeSystem.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "core/DomainEvents.h"
#include "core/EventBus.h"
#include "economy/Market.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"

void TradeSystem::passMarketHour(GameContext& ctx) const {
    ctx.time->spendHours(ctx.config.balance().tradeHours);
}

TradeResult TradeSystem::fail(TradeError c, const std::string& m,
                              int maxQty, int unitPrice) const {
    TradeResult r;
    r.ok = false;
    r.msg = m;
    r.code = c;
    r.maxQty = maxQty;
    r.unitPrice = unitPrice;
    return r;
}

TradeResult TradeSystem::buy(GameContext& ctx, const std::string& goodsId, int qty) const {
    const GoodsDef* g = cfg_.goods(goodsId);
    if (!g) return fail(TradeError::UnknownGoods, "没有这种商品: " + goodsId);
    if (qty <= 0) return fail(TradeError::InvalidCount, "数量必须为正");

    // 1) 地点：这里有没有卖
    const LocationDef* loc = cfg_.location(ctx.locationId);
    const bool soldHere =
        std::find(g->soldAt.begin(), g->soldAt.end(), ctx.locationId) != g->soldAt.end();
    if (!soldHere)
        return fail(TradeError::WrongLocation,
                    "「" + g->name + "」在" + (loc ? loc->name : ctx.locationId) + "没有卖");

    // 2) 报价（社交线折扣保留）
    int unit = ctx.market->buyPrice(ctx.locationId, goodsId);
    if (goodsId == "kaoyan_notes" && ctx.flags->has("senior_notes"))
        unit = std::max(1, static_cast<int>(unit * cfg_.balance().kaoyanDiscount));

    // 3) 三个上限：限购 / 容量 / 现金
    const int quota   = ctx.market->quotaLeft(ctx.locationId, goodsId);
    const int fit     = ctx.player->inventory().freeSlots() / g->slots;
    const int afford  = unit > 0 ? ctx.player->wallet().get() / unit : 0;
    const int maxQty  = std::min({quota, fit, afford});

    // 4) 一件都买不了：说清是哪条卡住
    if (maxQty <= 0) {
        if (quota <= 0)
            return fail(TradeError::QuotaExceeded,
                        "「" + g->name + "」今天在这里的限购额度已用完，明天再来", 0, unit);
        if (fit <= 0)
            return fail(TradeError::NotEnoughSlot, "背包塞不下了，先卖点东西吧", 0, unit);
        return fail(TradeError::NotEnoughCash, "现金不够，先打工或者卖货吧", 0, unit);
    }

    // 5) 买不到请求的数量：拒绝（不再静默减量）
    if (qty > maxQty) {
        const bool byQuota = (quota == maxQty);
        const bool bySlot  = (!byQuota && fit == maxQty);
        const TradeError code = byQuota ? TradeError::QuotaExceeded
                              : bySlot  ? TradeError::NotEnoughSlot
                                        : TradeError::NotEnoughCash;
        const char* why = byQuota ? "限购只剩 " : bySlot ? "背包只放得下 " : "现金只够买 ";
        return fail(code, std::string("最多可买 ") + std::to_string(maxQty) + " 件（" +
                          why + std::to_string(maxQty) + "）", maxQty, unit);
    }

    // 6) 成交（跑一趟市场：1 小时）
    const int total = unit * qty;
    ctx.player->wallet().spend(total);
    ctx.player->inventory().add(goodsId, qty, static_cast<double>(unit), g->slots);
    ctx.market->consumeQuota(ctx.locationId, goodsId, qty);
    passMarketHour(ctx);
    ctx.bus.publish(TradeCompletedEvent{goodsId, qty, total, true});

    TradeResult r;
    r.ok = true;
    r.msg = "买入「" + g->name + "」x" + std::to_string(qty) +
            "，支出 ¥" + std::to_string(total);
    r.amount = total;
    r.unitPrice = unit;
    r.maxQty = qty;          // 成功时也填，UI 可以统一显示
    return r;
}

TradeResult TradeSystem::sell(GameContext& ctx, const std::string& goodsId, int qty) const {
    const GoodsDef* g = cfg_.goods(goodsId);
    if (!g) return fail(TradeError::UnknownGoods, "没有这种商品: " + goodsId);
    if (qty <= 0) return fail(TradeError::InvalidCount, "数量必须为正");

    const Inventory::Item* item = ctx.player->inventory().find(goodsId);
    if (!item)
        return fail(TradeError::NotOwned, "背包里没有「" + g->name + "」");

    int unit = ctx.market->sellPrice(ctx.locationId, goodsId);
    // 社交线加成：学姐内部票（演出门票卖出 +25%）
    if (goodsId == "show_ticket" && ctx.flags->has("inner_ticket"))
        unit = std::max(1, static_cast<int>(unit * cfg_.balance().innerTicketBonus));

    if (qty > item->qty)
        return fail(TradeError::NotOwned,
                    "你只有 " + std::to_string(item->qty) + " 件「" + g->name + "」",
                    item->qty, unit);

    const int total = unit * qty;
    const double costBasis = item->avgCost * qty;   // 必须在 remove 之前取
    ctx.player->wallet().add(total);
    ctx.player->inventory().remove(goodsId, qty);
    passMarketHour(ctx); // 成交一趟市场：1 小时
    ctx.bus.publish(TradeCompletedEvent{goodsId, qty, total, false});

    const long long profit = static_cast<long long>(total) - static_cast<long long>(costBasis);
    TradeResult r;
    r.ok = true;
    r.msg = "卖出「" + g->name + "」x" + std::to_string(qty) +
            "，收入 ¥" + std::to_string(total);
    if (profit >= 0) r.msg += "（盈利 ¥" + std::to_string(profit) + "）";
    else             r.msg += "（亏损 ¥" + std::to_string(-profit) + "）";
    r.amount = total;
    r.unitPrice = unit;
    r.maxQty = qty;
    return r;
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
