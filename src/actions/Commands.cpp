#include "actions/Commands.h"

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "events/Effect.h"
#include "player/Player.h"
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
    const auto& jobs = ctx.config.balance().jobs;
    auto it = jobs.find(ctx.locationId);
    int lo = 80, hi = 100;
    std::string jobName = "打零工";
    if (it != jobs.end()) {
        lo = it->second.first;
        hi = it->second.second;
        if (ctx.locationId == "canteen") jobName = "食堂窗口帮工";
        else if (ctx.locationId == "store") jobName = "教育超市理货";
        else if (ctx.locationId == "westgate") jobName = "街头发传单";
    }
    int pay = ctx.rng.range(lo, hi);
    // 学长线回报：好感 ≥30，日薪 +20%（分工 3.5）
    if (ctx.npcs->atLeast("senior", 30)) pay = static_cast<int>(pay * ctx.config.balance().seniorWorkBonus);
    ctx.player->wallet().add(pay);
    return {true, true, jobName + "干了一天，工资 +¥" + std::to_string(pay)};
}

ActionResult StudyCommand::execute(GameContext& ctx) const {
    if (ctx.locationId != "library") return {false, false, "学习要去图书馆"};
    int gain = ctx.config.balance().studyGain;
    bool examSeason = ctx.time->week() >= ctx.config.balance().examStartWeek;
    if (examSeason) gain += ctx.config.balance().examStudyBonus; // 考试周冲刺加成
    ctx.player->addStudy(gain);
    ctx.time->markStudied();
    std::string flavor = examSeason ? "考试周冲刺！" : "泡了一天图书馆";
    return {true, true, flavor + "，平时分 +" + std::to_string(gain) +
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
