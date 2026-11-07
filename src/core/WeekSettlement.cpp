#include "core/WeekSettlement.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "core/DomainEvents.h"
#include "core/EventBus.h"
#include "core/GameContext.h"
#include "economy/Market.h"
#include "events/EventScheduler.h"
#include "player/Player.h"
#include "story/StoryFlags.h"
#include "stock/StockSystem.h"
#include "time/TimeSystem.h"
#include "util/JsonUtil.h"

WeekSettlement::Report WeekSettlement::run() {
    rep_ = Report{};
    rep_.week = ctx_.time->week();
    rep_.moneyBefore = ctx_.player->wallet().get();

    chargeExpenses();
    settleStudy();
    applyScheduledEffects();
    settleStocks();
    settleEnergy();
    checkMilestones();
    beginNewWeek();
    refreshMarketAndRumors();

    rep_.moneyAfter = ctx_.player->wallet().get();
    rep_.semesterOver = ctx_.time->isSemesterOver();
    return rep_;
}

void WeekSettlement::chargeExpenses() {
    int expense = std::min(ctx_.config.balance().weeklyExpense, ctx_.player->wallet().get());
    if (expense > 0) ctx_.player->wallet().spend(expense);
    rep_.lines.push_back("生活费支出 -¥" + std::to_string(expense) +
                         "（余额 ¥" + std::to_string(ctx_.player->wallet().get()) + "）");
}

void WeekSettlement::settleStudy() {
    if (ctx_.time->week() > 1 && !ctx_.time->studiedThisWeek()) {
        ctx_.player->addStudy(-ctx_.config.balance().studyDecay);
        rep_.lines.push_back("这周一次都没学习，平时分 -" +
                             std::to_string(ctx_.config.balance().studyDecay) +
                             "（当前 " + std::to_string(ctx_.player->studyScore()) + "）");
    }
}

void WeekSettlement::applyScheduledEffects() {
    // 延迟效果到期的本周生效（如借室友的钱被归还）
    ctx_.events->applyDueEffects(ctx_);
}

void WeekSettlement::settleStocks() {
    // 融资负债按周计息（先计息再走价格，爆仓在此触发）
    long long interest = ctx_.stock->chargeWeeklyInterest();
    if (interest > 0)
        rep_.lines.push_back("【股市】融资利息 -¥" + std::to_string(interest) +
                             "（负债按周计息）");
    ctx_.stock->onTick(ctx_); // 周日股价也波动一格
}

void WeekSettlement::settleEnergy() {
    ctx_.player->energy().add(ctx_.config.balance().staminaWeeklyRecover);
}

void WeekSettlement::checkMilestones() {
    const BalanceDef& b = ctx_.config.balance();
    int week = ctx_.time->week();
    int score = ctx_.player->studyScore();

    if (week == b.midtermWeek && score < b.warnScore) {
        rep_.lines.push_back("【期中警告】平时分 " + std::to_string(score) + " 低于警戒线 " +
                             std::to_string(b.warnScore) + "，期末再不冲刺要挂科了！");
    }
    if (week == b.weeks) { // 期末周（第 18 周）结算
        if (score < b.passScore) {
            ctx_.flags->set("failed");
            int fee = std::min(b.retakeFee, ctx_.player->wallet().get());
            if (fee > 0) ctx_.player->wallet().spend(fee);
            rep_.lines.push_back("【期末】平时分 " + std::to_string(score) + "，挂科了……补考费 -¥" +
                                 std::to_string(fee));
        } else {
            rep_.lines.push_back("【期末】平时分 " + std::to_string(score) + "，稳过！");
            if (score >= b.scholarshipScore) {
                ctx_.player->wallet().add(b.scholarship);
                rep_.lines.push_back("【奖学金】平时分 ≥" + std::to_string(b.scholarshipScore) +
                                     "，获得奖学金 +¥" + std::to_string(b.scholarship));
            }
        }
    }
}

void WeekSettlement::beginNewWeek() {
    ctx_.time->startNextWeek();
    ctx_.events->onWeekStart(); // 事件冷却递减
    ctx_.bus.publish(WeekStartedEvent{ctx_.time->week()});
}

void WeekSettlement::refreshMarketAndRumors() {
    int newWeek = ctx_.time->week();
    ctx_.market->syncWeek(newWeek);
    ctx_.market->onWeekStart();   // 新一周趋势（消化上周的情报）
    ctx_.market->rollRumor();     // 摇下周的新情报
    ctx_.rumors = ctx_.market->rumorTexts();
    if (!ctx_.rumors.empty())
        rep_.lines.push_back(ctx_.rumors.front());
}
