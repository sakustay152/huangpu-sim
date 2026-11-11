#include "player/GoalTracker.h"

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "player/Energy.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "story/NpcSystem.h"
#include "time/TimeSystem.h"

GoalSnapshot GoalTracker::snapshot(const GameContext& ctx) const {
    GoalSnapshot s;
    const BalanceDef& b = ctx.config.balance();

    s.cash = ctx.player->wallet().get();
    s.inventoryValue = ctx.trade->estimatePortfolioValue(ctx);  // 别重写公式
    s.totalAssets = s.cash + s.inventoryValue;
    s.targetMoney = b.targetMoney;

    s.studyScore = ctx.player->studyScore();
    s.passScore = b.passScore;
    s.failed = ctx.player->study().failed();
    s.maxAffinity = ctx.npcs->maxAffinity();

    s.week = ctx.time->week();
    s.weeks = b.weeks;
    s.day = ctx.time->day();
    s.hour = ctx.time->hour();

    s.stamina = ctx.player->energy().get();
    s.staminaMax = ctx.player->energy().max();
    s.slotsUsed = ctx.player->inventory().usedSlots();
    s.slotsMax = ctx.player->inventory().capacity();
    return s;
}
