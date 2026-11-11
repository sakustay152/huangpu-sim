#include "core/Session.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "core/DomainEvents.h"
#include "core/EventBus.h"
#include "core/GameContext.h"
#include "core/IEffect.h"
#include "core/ITimeListener.h"
#include "core/Random.h"
#include "economy/Market.h"
#include "events/GameEvent.h"
#include "events/EventScheduler.h"
#include "player/Player.h"
#include "stock/StockSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"

ActionOutcome Session::runAction(GameContext& ctx, const ICommand& cmd) {
    ActionOutcome out;
    if (!ctx.time->canAct()) {
        out.result = {false, false, "凌晨 2 点的困意压垮了你……（昏迷边缘，先睡觉）"};
        return out;
    }
    const int weekBefore = ctx.time->week();
    const int dayBefore = ctx.time->day();

    ActionResult r = cmd.execute(ctx); // 命令内部扣除体力与小时数
    out.result = r;
    if (!(r.ok && r.consumedTime)) return out;

    // 股价随每次行动走一根 K 线（持仓爆仓会在此触发）
    ctx.stock->onTick(ctx);
    ctx.bus.publish(ActionTakenEvent{cmd.name()});

    // ---- 昏迷判定：熬过凌晨 2 点（hour >= 26）→ 统一入口处理 ----
    bool dayChanged = false;
    {
        std::string faintMsg;
        out.weekAdvanced = ctx.handleFaintAndAdvance(faintMsg);
        if (!faintMsg.empty()) {
            r.msg += faintMsg;
            out.result.msg = r.msg;
            out.fainted = true;
            dayChanged = true;
        }
    }

    // ---- 跨天（睡觉/昏迷推进）：行情按天刷新 ----
    if (ctx.time->day() != dayBefore || ctx.time->week() != weekBefore) dayChanged = true;
    if (dayChanged) {
        ITimeListener& marketListener = *ctx.market;
        marketListener.onDayTick(); // 限购/噪声按天刷新
    }
    if (ctx.time->week() != weekBefore) out.weekAdvanced = true;

    // 概率摇随机事件（数据驱动，分工 8.2）
    if (ctx.rng.chance(ctx.config.balance().eventChance)) {
        if (const GameEvent* ev = ctx.events->rollRandom(ctx)) out.event = ev;
    }
    return out;
}

std::string Session::applyChoice(GameContext& ctx, const GameEvent& ev, int choiceIdx) {
    if (ev.choices.empty()) return "";
    if (choiceIdx < 0 || choiceIdx >= static_cast<int>(ev.choices.size())) choiceIdx = 0;
    const auto& choice = ev.choices[static_cast<std::size_t>(choiceIdx)];

    std::string out = "你选择了：" + choice.text;
    for (const auto& effect : choice.effects) {
        std::string msg = effect->apply(ctx);
        if (!msg.empty()) out += "\n  · " + msg;
    }
    ctx.flags->set("evt_done_" + ev.id); // 一次性事件标记（GameEvent::canTrigger 读取）
    ctx.events->markTriggered(ev);       // 随机事件冷却登记
    return out;
}
