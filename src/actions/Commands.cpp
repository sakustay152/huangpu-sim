#include "actions/Commands.h"

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "events/Effect.h"
#include "player/JobSystem.h"
#include "player/Player.h"
#include "player/StudySystem.h"
#include "story/NpcSystem.h"
#include "time/TimeSystem.h"

ActionResult MoveCommand::execute(GameContext& ctx) const {
    const LocationDef* to = ctx.config.location(to_);
    if (!to) return {false, false, "没有这个地方: " + to_};
    if (to->id == ctx.locationId) return {false, false, "你已经在这里了"};
    int fare = to->fare;
    if (!ctx.player->wallet().spend(fare))
        return {false, false, "连 ¥" + std::to_string(fare) + " 路费都掏不出来，先打工吧"};
    ctx.locationId = to->id;
    return {true, true, "移动到【" + to->name + "】（路费 -¥" + std::to_string(fare) + "）：" + to->desc};
}

ActionResult WorkCommand::execute(GameContext& ctx) const {
    JobResult r = JobSystem(ctx.config).work(ctx);
    return {r.ok, true, r.msg};
}

ActionResult StudyCommand::execute(GameContext& ctx) const {
    if (ctx.locationId != "library") return {false, false, "学习要去图书馆"};

    std::string msg;
    const int gain = ctx.player->study().study(mode_, ctx.player->energy(),
                                               ctx.time->week(), msg);
    ctx.time->markStudied();
    return {true, true, msg + "，平时分 +" + std::to_string(gain) +
                            "（当前 " + std::to_string(ctx.player->studyScore()) + "）"};
}

ActionResult SocialCommand::execute(GameContext& ctx) const {
    const LocationDef* loc = ctx.config.location(ctx.locationId);
    if (!loc || loc->npcId.empty())
        return {false, false, "这里没有能社交的人"};
    // 复用 AffinityEffect（含门槛解锁提示）—— 效果系统一处实现，多处复用
    AffinityEffect effect(loc->npcId, ctx.config.balance().socialGain);
    std::string detail = effect.apply(ctx);
    return {true, true, "和" + (ctx.config.npc(loc->npcId) ? ctx.config.npc(loc->npcId)->name : "朋友") +
                            "聊了一晚上：" + detail};
}

ActionResult RestCommand::execute(GameContext& ctx) const {
    if (ctx.locationId != "dorm") return {false, false, "回宿舍才能休息"};
    ctx.player->energy().add(ctx.config.balance().staminaRestGain); // 炒股耗的体力在这里回来
    AffinityEffect effect("roommate", ctx.config.balance().restAffinity);
    std::string detail = effect.apply(ctx);
    return {true, true, "睡了个好觉（体力 +" +
                            std::to_string(ctx.config.balance().staminaRestGain) + "），和室友卧谈到半夜：" + detail};
}

ActionResult LearnStockCommand::execute(GameContext& ctx) const {
    const BalanceDef& b = ctx.config.balance();
    if (!ctx.player->energy().spend(b.learnStockStaminaCost))
        return {false, false, "精力不支，看不进书……（学股市知识需要 " +
                                  std::to_string(b.learnStockStaminaCost) +
                                  " 点体力，回宿舍休息恢复）"};
    int gain = ctx.rng.range(b.learnStockGainMin, b.learnStockGainMax);
    int before = ctx.player->stockSkill();
    ctx.player->addStockSkill(gain);
    int after = ctx.player->stockSkill();
    std::string msg = "啃了一天 K 线教材（体力 -" + std::to_string(b.learnStockStaminaCost) +
                      "），股力 +" + std::to_string(after - before) +
                      "（" + std::to_string(before) + "→" + std::to_string(after) + "）";
    if (before < b.stockSkillMax && after >= b.stockSkillMax)
        msg += "｜你对庄家的套路已了如指掌！";
    return {true, true, msg};
}
