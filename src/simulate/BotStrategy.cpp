#include "simulate/BotStrategy.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "actions/Commands.h"
#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "core/IEffect.h"
#include "core/Session.h"
#include "economy/Market.h"
#include "events/GameEvent.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "story/NpcSystem.h"
#include "time/TimeSystem.h"

namespace {

// 简单解析器：选"估算现金收益最大"的选项（IEffect::estimateMoneyDelta 多态求和）
class SimResolver : public IChoiceResolver {
public:
    int chooseChoice(const GameEvent& ev, const GameContext& ctx) const override {
        (void)ctx;
        int best = 0;
        long long bestValue = std::numeric_limits<long long>::min();
        for (std::size_t i = 0; i < ev.choices.size(); ++i) {
            long long v = 0;
            for (const auto& e : ev.choices[i].effects) v += e->estimateMoneyDelta();
            if (v > bestValue) {
                bestValue = v;
                best = static_cast<int>(i);
            }
        }
        return best;
    }
};

// 执行一条消耗行动点的命令；途中触发的随机事件由 resolver 立即解决
bool runApCommand(GameContext& ctx, const ICommand& cmd, const IChoiceResolver& resolver) {
    ActionOutcome out = Session::runAction(ctx, cmd);
    if (out.event) Session::applyChoice(ctx, *out.event, resolver.chooseChoice(*out.event, ctx));
    return out.result.ok && out.result.consumedAp;
}

// 某商品在各地的最高卖价（及地点 id）
std::pair<int, std::string> bestSell(GameContext& ctx, const std::string& goodsId) {
    int best = 0;
    std::string bestLoc;
    for (const auto& l : ctx.config.locations()) {
        int p = ctx.market->sellPrice(l.id, goodsId);
        if (p > best) {
            best = p;
            bestLoc = l.id;
        }
    }
    return {best, bestLoc};
}

// ---------------- 只倒卖：低买高卖，周内周转 ----------------
class TraderBot : public BotStrategy {
public:
    std::string name() const override { return "trader"; }
    bool takeTurn(GameContext& ctx) override {
        if (!ctx.time->canAct()) return false;

        // 学业底线：期中前把分数拉到 58 以上
        if (ctx.time->week() >= 5 && ctx.player->studyScore() < 58) {
            if (ctx.locationId != "library") return runApCommand(ctx, MoveCommand("library"), resolver_);
            return runApCommand(ctx, StudyCommand(), resolver_);
        }
        // 1) 有库存 → 去最优地点卖
        const auto& items = ctx.player->inventory().items();
        if (!items.empty()) {
            const Inventory::Item* bestItem = nullptr;
            int bestProfit = -1;
            std::string bestLoc;
            for (const auto& it : items) {
                auto [p, loc] = bestSell(ctx, it.goodsId);
                int profit = (p - static_cast<int>(it.avgCost)) * it.qty;
                if (profit > bestProfit) {
                    bestProfit = profit;
                    bestItem = &it;
                    bestLoc = loc;
                }
            }
            if (bestItem && bestProfit > 0) {
                if (ctx.locationId == bestLoc) {
                    // 就地清仓所有"此地即最优"的货（免行动点）
                    for (const auto& it : items) {
                        if (bestSell(ctx, it.goodsId).second != ctx.locationId) continue;
                        const Inventory::Item* cur = ctx.player->inventory().find(it.goodsId);
                        if (cur) ctx.trade->sell(ctx, it.goodsId, cur->qty);
                    }
                    return false;
                }
                return runApCommand(ctx, MoveCommand(bestLoc), resolver_);
            }
        }

        // 2) 扫描进货机会：毛利/占格 最大者
        int bestMargin = -1;
        std::string bestGoods, bestBuyLoc;
        for (const auto& g : ctx.config.goods()) {
            if (!ctx.player->inventory().canFit(1, g.slots)) continue;
            auto [sellP, sellLoc] = bestSell(ctx, g.id);
            for (const auto& buyLocId : g.soldAt) {
                int buyP = ctx.market->buyPrice(buyLocId, g.id);
                if (ctx.market->quotaLeft(buyLocId, g.id) <= 0) continue;
                int margin = sellP - buyP;
                if (margin <= 0) continue;
                int marginPerSlot = margin / g.slots;
                if (marginPerSlot > bestMargin) {
                    bestMargin = marginPerSlot;
                    bestGoods = g.id;
                    bestBuyLoc = buyLocId;
                }
            }
        }
        const GoodsDef* bg = bestGoods.empty() ? nullptr : ctx.config.goods(bestGoods);
        if (bg) {
            int buyP = ctx.market->buyPrice(bestBuyLoc, bestGoods);
            auto [sellP, sellLoc] = bestSell(ctx, bestGoods);
            double ratio = buyP > 0 ? static_cast<double>(sellP - buyP) / buyP : 0.0;
            if (ratio > 0.12 && sellLoc != bestBuyLoc) { // 12% 毛利才出手，防止无意义倒手
                if (ctx.locationId == bestBuyLoc) {
                    int affordable = buyP > 0 ? ctx.player->wallet().get() / buyP : 0;
                    int fit = ctx.player->inventory().freeSlots() / bg->slots;
                    int qty = std::min({ctx.market->quotaLeft(bestBuyLoc, bestGoods), affordable, fit});
                    if (qty > 0) {
                        ctx.trade->buy(ctx, bestGoods, qty);
                        return false;
                    }
                }
                return runApCommand(ctx, MoveCommand(bestBuyLoc), resolver_);
            }
        }

        // 3) 兜底：打工
        return runApCommand(ctx, WorkCommand(), resolver_);
    }

private:
    SimResolver resolver_;
};

// ---------------- 只打工：稳定但天花板低（验证"单一策略不优"） ----------------
class WorkerBot : public BotStrategy {
public:
    std::string name() const override { return "worker"; }
    bool takeTurn(GameContext& ctx) override {
        if (!ctx.time->canAct()) return false;
        // 学业及格线策略（保住 60 分即可，不为奖学金浪费行动点）
        if (ctx.player->studyScore() < 62 ||
            (ctx.time->week() >= 6 && !ctx.time->studiedThisWeek() && ctx.player->studyScore() < 70)) {
            if (ctx.locationId != "library") return runApCommand(ctx, MoveCommand("library"), resolver_);
            return runApCommand(ctx, StudyCommand(), resolver_);
        }
        // 其余时间打工
        return runApCommand(ctx, WorkCommand(), resolver_);
    }

private:
    SimResolver resolver_;
};

// ---------------- 混合：打工攒本 + 高毛利倒卖 + 兼顾学业 ----------------
class MixedBot : public BotStrategy {
public:
    std::string name() const override { return "mixed"; }
    bool takeTurn(GameContext& ctx) override {
        if (!ctx.time->canAct()) return false;

        // 学业：稳在及格线上方（不冲奖学金——行动点拿去经营更划算）
        if (ctx.player->studyScore() < 70 ||
            (ctx.time->week() >= 6 && !ctx.time->studiedThisWeek() && ctx.player->studyScore() < 74)) {
            if (ctx.locationId != "library") return runApCommand(ctx, MoveCommand("library"), resolver_);
            return runApCommand(ctx, StudyCommand(), resolver_);
        }

        // 清仓（同 TraderBot）
        const auto& items = ctx.player->inventory().items();
        if (!items.empty()) {
            std::string bestLoc;
            int bestProfit = -1;
            for (const auto& it : items) {
                auto [p, loc] = bestSell(ctx, it.goodsId);
                int profit = (p - static_cast<int>(it.avgCost)) * it.qty;
                if (profit > bestProfit) {
                    bestProfit = profit;
                    bestLoc = loc;
                }
            }
            if (bestProfit > 0) {
                if (ctx.locationId == bestLoc) {
                    for (const auto& it : items) {
                        if (bestSell(ctx, it.goodsId).second != ctx.locationId) continue;
                        const Inventory::Item* cur = ctx.player->inventory().find(it.goodsId);
                        if (cur) ctx.trade->sell(ctx, it.goodsId, cur->qty);
                    }
                    return false;
                }
                return runApCommand(ctx, MoveCommand(bestLoc), resolver_);
            }
        }

        // 社交底线：好结局需要一位好友（好感 ≥60），空闲时维护到 80
        bool urgentSocial = ctx.npcs->maxAffinity() < 60;
        bool casualSocial = ctx.npcs->maxAffinity() < 80;
        if (urgentSocial || casualSocial) {
            const LocationDef* loc = ctx.config.location(ctx.locationId);
            bool npcHere = loc && !loc->npcId.empty();
            if (npcHere && (urgentSocial || !hasGoodBuyOpportunity(ctx)))
                return runApCommand(ctx, SocialCommand(), resolver_);
            if (urgentSocial) return runApCommand(ctx, MoveCommand("dorm"), resolver_);
        }

        // 本金少先打工攒本
        if (ctx.player->wallet().get() < 1200) return runApCommand(ctx, WorkCommand(), resolver_);

        // 高毛利才进货（13% 门槛，比纯倒卖略保守）
        int bestMargin = -1;
        std::string bestGoods, bestBuyLoc;
        for (const auto& g : ctx.config.goods()) {
            if (!ctx.player->inventory().canFit(1, g.slots)) continue;
            auto [sellP, sellLoc] = bestSell(ctx, g.id);
            if (sellLoc.empty() || sellLoc == "?") continue;
            for (const auto& buyLocId : g.soldAt) {
                if (buyLocId == sellLoc) continue; // 混合策略不原地倒手
                int buyP = ctx.market->buyPrice(buyLocId, g.id);
                if (ctx.market->quotaLeft(buyLocId, g.id) <= 0) continue;
                int marginPerSlot = (sellP - buyP) / g.slots;
                if (marginPerSlot > bestMargin) {
                    bestMargin = marginPerSlot;
                    bestGoods = g.id;
                    bestBuyLoc = buyLocId;
                }
            }
        }
        const GoodsDef* bg = bestGoods.empty() ? nullptr : ctx.config.goods(bestGoods);
        if (bg) {
            int buyP = ctx.market->buyPrice(bestBuyLoc, bestGoods);
            auto [sellP, sellLoc] = bestSell(ctx, bestGoods);
            double ratio = buyP > 0 ? static_cast<double>(sellP - buyP) / buyP : 0.0;
            if (ratio > 0.13) {
                if (ctx.locationId == bestBuyLoc) {
                    int affordable = buyP > 0 ? ctx.player->wallet().get() / buyP : 0;
                    int fit = ctx.player->inventory().freeSlots() / bg->slots;
                    int qty = std::min({ctx.market->quotaLeft(bestBuyLoc, bestGoods), affordable, fit});
                    if (qty > 0) {
                        ctx.trade->buy(ctx, bestGoods, qty);
                        return false;
                    }
                }
                return runApCommand(ctx, MoveCommand(bestBuyLoc), resolver_);
            }
        }
        return runApCommand(ctx, WorkCommand(), resolver_);
    }

private:
    SimResolver resolver_;

    // 是否存在值得出手的进货机会（毛利 ≥13%）
    bool hasGoodBuyOpportunity(GameContext& ctx) const {
        for (const auto& g : ctx.config.goods()) {
            if (!ctx.player->inventory().canFit(1, g.slots)) continue;
            auto [sellP, sellLoc] = bestSell(ctx, g.id);
            if (sellLoc.empty()) continue;
            for (const auto& buyLocId : g.soldAt) {
                if (buyLocId == sellLoc) continue;
                int buyP = ctx.market->buyPrice(buyLocId, g.id);
                if (buyP > 0 && ctx.market->quotaLeft(buyLocId, g.id) > 0 &&
                    static_cast<double>(sellP - buyP) / buyP > 0.13)
                    return true;
            }
        }
        return false;
    }
};

} // namespace

const IChoiceResolver& defaultResolver() {
    static SimResolver r;
    return r;
}

std::unique_ptr<BotStrategy> makeStrategy(const std::string& name) {
    if (name == "trader") return std::make_unique<TraderBot>();
    if (name == "worker") return std::make_unique<WorkerBot>();
    if (name == "mixed") return std::make_unique<MixedBot>();
    throw std::runtime_error("未知策略: " + name + "（可选 trader/worker/mixed）");
}
