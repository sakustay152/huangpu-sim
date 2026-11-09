#include "player/JobSystem.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "player/Energy.h"
#include "player/Player.h"
#include "story/NpcSystem.h"

JobResult JobSystem::work(GameContext& ctx) const {
    const BalanceDef& b = ctx.config.balance();

    auto jobIt = b.jobs.find(ctx.locationId);
    const int lo = (jobIt == b.jobs.end()) ? 80  : jobIt->second.first;
    const int hi = (jobIt == b.jobs.end()) ? 100 : jobIt->second.second;

    JobResult r;
    auto nameIt = b.jobNames.find(ctx.locationId);
    r.jobName = (nameIt != b.jobNames.end()) ? nameIt->second : "打零工";

    r.pay = ctx.rng.range(lo, hi);
    if (ctx.npcs->atLeast("senior", 30))
        r.pay = static_cast<int>(r.pay * b.seniorWorkBonus);

    r.lowEnergy = ctx.player->energy().isLow();          // < 30
    if (r.lowEnergy) r.pay = static_cast<int>(r.pay * b.lowEnergyPayRate);

    auto stIt = b.jobStamina.find(ctx.locationId);
    const int cost = (stIt != b.jobStamina.end()) ? stIt->second : 20;
    r.staminaCost = std::min(cost, ctx.player->energy().get()); // 扣到 0 为止，不为负
    ctx.player->energy().spend(r.staminaCost);
    ctx.player->wallet().add(r.pay);

    r.msg = r.jobName + "干了一天，工资 +¥" + std::to_string(r.pay);
    if (r.lowEnergy) r.msg += "（体力不支，只干出半份活）";
    return r;
}
