#include "events/GameEvent.h"

#include "core/GameContext.h"
#include "core/IEffect.h"
#include "events/Condition.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"

bool GameEvent::canTrigger(const GameContext& ctx) const {
    if (once && ctx.flags->has("evt_done_" + id)) return false;
    for (const auto& c : conditions)
        if (!c->test(ctx)) return false;
    return true;
}

void GameEvent::apply(GameContext& ctx) const {
    if (choices.empty()) return;
    for (const auto& e : choices.front().effects) e->apply(ctx);
    ctx.flags->set("evt_done_" + id);
}

bool FixedEvent::canTrigger(const GameContext& ctx) const {
    if (ctx.time->week() != week) return false;
    return GameEvent::canTrigger(ctx);
}

bool NpcEvent::canTrigger(const GameContext& ctx) const {
    if (!ctx.npcs->atLeast(npcId, minAffinity)) return false;
    return GameEvent::canTrigger(ctx);
}
