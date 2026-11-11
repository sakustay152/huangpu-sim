#include "actions/Commands.h"

#include <algorithm>

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
    const BalanceDef& b = ctx.config.balance();
    const LocationDef* to = ctx.config.location(to_);
    if (!to) return {false, false, "没有这个地方: " + to_};
    if (to->id == ctx.locationId) return {false, false, "你已经在这里了"};
    if (!ctx.player->energy().spend(b.moveStamina))
        return {false, false, "体力不支，走不动了……（移动需要 " +
                                  std::to_string(b.moveStamina) + " 点体力，回宿舍睡一觉）"};
    ctx.time->spendHours(b.moveHours);
    ctx.locationId = to->id;
    return {true, true, "移动到【" + to->name + "】（体力 -" + std::to_string(b.moveStamina) +
                            " · " + std::to_string(b.moveHours) + " 小时）：" + to->desc};
}

ActionResult WorkCommand::execute(GameContext& ctx) const {
    const BalanceDef& b = ctx.config.balance();
    JobResult r = JobSystem(ctx.config).work(ctx);
    ctx.time->spendHours(b.workHours);
    return {r.ok, true, r.msg + "（连轴 " + std::to_string(b.workHours) + " 小时班）"};
}

ActionResult StudyCommand::execute(GameContext& ctx) const {
    const BalanceDef& b = ctx.config.balance();
    if (ctx.locationId != "library") return {false, false, "学习要去图书馆"};
    if (!ctx.player->energy().spend(b.studyStamina))
        return {false, false, "体力不支，看不进书……（学习需要 " +
                                  std::to_string(b.studyStamina) + " 点体力，回宿舍睡一觉）"};
    std::string msg;
    const int gain = ctx.player->study().study(mode_, ctx.player->energy(),
                                               ctx.time->week(), msg);
    ctx.time->spendHours(b.studyHours);
    ctx.time->markStudied();
    return {true, true, msg + "，平时分 +" + std::to_string(gain) +
                            "（当前 " + std::to_string(ctx.player->studyScore()) + "）"};
}

ActionResult SleepCommand::execute(GameContext& ctx) const {
    const BalanceDef& b = ctx.config.balance();
    if (ctx.locationId != "dorm")
        return {false, false, "只能回宿舍睡觉（在外面熬到凌晨 2 点会昏迷进校医院）"};

    // 睡眠时长：从当前时刻睡到次日 8:00
    int duration = 32 - ctx.time->hour();
    std::string extra;
    if (ctx.rng.chance(b.insomniaChance)) {
        duration = std::max(1, duration - b.insomniaPenaltyHours);
        extra = "｜失眠了……辗转反侧，少睡了 " +
                std::to_string(b.insomniaPenaltyHours) + " 个小时";
    }

    const std::string bedClock = ctx.time->clockText(); // 睡前时刻（advanceDay 前取）
    int recovered = 0;
    if (duration >= b.sleepFullHours) {
        recovered = ctx.player->energy().max() - ctx.player->energy().get();
        ctx.player->energy().restoreFull(); // 睡满 8 小时强制回满
    } else {
        recovered = duration * ctx.player->energy().max() / b.sleepFullHours;
        ctx.player->energy().add(recovered);
    }

    const bool weekRolled = ctx.time->advanceDay(); // 推进到次日 8:00（周日→周一触发周结算）
    std::string msg = "你在 " + bedClock + " 睡下，一觉到早上 8:00（体力 +" +
                      std::to_string(recovered) + "）" + extra;
    if (weekRolled) msg += "｜本周结束，周日结算见。";
    return {true, true, msg};
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
    ctx.time->spendHours(b.learnStockHours);
    std::string msg = "啃了 " + std::to_string(b.learnStockHours) +
                      " 个小时 K 线教材（体力 -" + std::to_string(b.learnStockStaminaCost) +
                      "），股力 +" + std::to_string(after - before) +
                      "（" + std::to_string(before) + "→" + std::to_string(after) + "）";
    if (before < b.stockSkillMax && after >= b.stockSkillMax)
        msg += "｜你对庄家的套路已了如指掌！";
    return {true, true, msg};
}

ActionResult SocialPickCommand::execute(GameContext& ctx) const {
    const BalanceDef& b = ctx.config.balance();
    const LocationDef* loc = ctx.config.location(ctx.locationId);
    if (!loc || loc->npcId.empty() || loc->npcId != npcId_)
        return {false, false, "对方已经不在这里了"};
    const NpcDef* n = ctx.config.npc(npcId_);
    if (!n) return {false, false, "（无效的 NPC）"};
    if (!ctx.player->energy().spend(b.socialStamina))
        return {false, false, "体力不支，提不起劲聊天……（社交需要 " +
                                  std::to_string(b.socialStamina) + " 点体力）"};
    ctx.time->spendHours(b.socialHours);
    int before = ctx.npcs->affinity(npcId_);
    ctx.npcs->addAffinity(npcId_, delta_);
    int after = ctx.npcs->affinity(npcId_);
    std::string msg = "和" + n->name + "：「" + text_ + "」——好感 " +
                      (delta_ >= 0 ? "+" : "") + std::to_string(delta_) +
                      "（" + std::to_string(before) + "→" + std::to_string(after) + "）";
    for (int gate : {30, 60, 80}) {
        if (before < gate && after >= gate) {
            msg += "｜解锁新渠道！";
            if (npcId_ == "roommate" && gate == 30) msg += "（室友每周多给一条二手货情报）";
            if (npcId_ == "senior" && gate == 30) msg += "（学长介绍打工，日薪 +20%）";
            if (npcId_ == "senior" && gate == 60) msg += "（考研资料进货价 9 折）";
            if (npcId_ == "xuejie" && gate == 30) msg += "（学姐带你进活动圈子）";
            if (npcId_ == "xuejie" && gate == 60) msg += "（演出内部票渠道开通）";
        }
    }
    return {true, true, msg};
}
