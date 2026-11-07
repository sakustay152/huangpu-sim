#include "core/Session.h"

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
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"

ActionOutcome Session::runAction(GameContext& ctx, const ICommand& cmd) {
    ActionOutcome out;
    if (!ctx.time->canAct()) {
        out.result = {false, false, "本周行动点已用完（周日晚自动结算）"};
        return out;
    }
    ActionResult r = cmd.execute(ctx);
    out.result = r;
    if (!(r.ok && r.consumedAp)) return out;

    // 时间推进：一个行动点 = 一天
    ctx.time->spendAction();
    // 通过 ITimeListener 接口通知行情（面向接口，不面向实现）
    ITimeListener& marketListener = *ctx.market;
    marketListener.onDayTick();
    ctx.bus.publish(ActionTakenEvent{cmd.name()});

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
