#include "simulate/SimulationRunner.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

#include "core/ConfigManager.h"
#include "core/EventBus.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "core/Session.h"
#include "core/WeekSettlement.h"
#include "events/EndingJudge.h"
#include "events/EventScheduler.h"
#include "events/GameEvent.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "simulate/BotStrategy.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"

SimGameResult SimulationRunner::runOne(const std::string& strategyName, std::uint64_t seed) {
    Random rng(seed);
    EventBus bus;
    GameContext ctx(cfg_, rng, bus);
    ctx.initNewGame();

    auto bot = makeStrategy(strategyName);

    // 第 1 周固定节点立即解决（Bot 选"对自己最有利"的选项）
    for (const GameEvent* ev : ctx.events->fixedForWeek(1, ctx)) {
        Session::applyChoice(ctx, *ev, defaultResolver().chooseChoice(*ev, ctx));
    }

    int guard = 0;
    int stall = 0;
    int minMoney = ctx.player->wallet().get();
    while (!ctx.time->isSemesterOver() && guard++ < 20000) {
        bool consumed = bot->takeTurn(ctx);
        stall = consumed ? 0 : stall + 1;
        if (stall > 8) { // 兜底：策略卡住则强制进入周日结算
            ctx.time->forceEndWeek();
            stall = 0;
        }
        minMoney = std::min(minMoney, ctx.player->wallet().get());
        if (ctx.flags->has("bankrupt")) break; // 炒股爆仓破产（Bot 不炒股，防御性检查）
        if (!ctx.time->canAct()) {
            WeekSettlement settle(ctx);
            settle.run();
            if (!ctx.time->isSemesterOver()) {
                for (const GameEvent* ev : ctx.events->fixedForWeek(ctx.time->week(), ctx)) {
                    Session::applyChoice(ctx, *ev, defaultResolver().chooseChoice(*ev, ctx));
                }
            }
        }
    }

    SimGameResult r;
    r.seed = seed;
    r.strategy = strategyName;
    const EndingDef* ending = EndingJudge::judge(ctx);
    r.endingId = ending ? ending->id : "?";
    r.finalCash = ctx.player->wallet().get();
    r.portfolioValue = ctx.trade->estimatePortfolioValue(ctx);
    r.studyScore = ctx.player->studyScore();
    r.maxAffinity = ctx.npcs->maxAffinity();
    r.failed = ctx.flags->has("failed");
    r.minMoney = minMoney;
    return r;
}

std::vector<SimGameResult> SimulationRunner::runGames(const std::string& strategyName,
                                                      std::uint64_t seed0, int count) {
    std::vector<SimGameResult> out;
    out.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        out.push_back(runOne(strategyName, seed0 + static_cast<std::uint64_t>(i)));
    }
    return out;
}

void SimulationRunner::writeCsv(const std::string& path, const std::vector<SimGameResult>& results) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("无法写 CSV: " + path);
    f << "seed,strategy,ending,final_cash,portfolio_value,study_score,max_affinity,failed,min_money\n";
    for (const auto& r : results) {
        f << r.seed << ',' << r.strategy << ',' << r.endingId << ',' << r.finalCash << ','
          << r.portfolioValue << ',' << r.studyScore << ',' << r.maxAffinity << ','
          << (r.failed ? 1 : 0) << ',' << r.minMoney << '\n';
    }
}

void SimulationRunner::printSummary(const std::vector<SimGameResult>& results) {
    // 按策略聚合
    std::map<std::string, std::vector<const SimGameResult*>> byStrategy;
    for (const auto& r : results) byStrategy[r.strategy].push_back(&r);

    std::cout << "\n==== 1000 局模拟摘要（同一套规则，无 UI）====\n";
    std::cout << "策略      局数  校园合伙人  站稳脚跟  账单之外  平均总资产   中位总资产   挂科率  平均平时分  最低现金均值\n";
    for (auto& [name, rs] : byStrategy) {
        std::map<std::string, int> endings;
        long long totalSum = 0;
        std::vector<int> totals;
        int failed = 0;
        long long studySum = 0;
        long long minMoneySum = 0;
        for (const auto* r : rs) {
            endings[r->endingId]++;
            int total = r->finalCash + r->portfolioValue;
            totalSum += total;
            totals.push_back(total);
            studySum += r->studyScore;
            minMoneySum += r->minMoney;
            if (r->failed) ++failed;
        }
        std::sort(totals.begin(), totals.end());
        int median = totals.empty() ? 0 : totals[totals.size() / 2];
        auto rate = [&](const char* id) {
            auto it = endings.find(id);
            double v = it == endings.end() ? 0.0 : static_cast<double>(it->second) / rs.size();
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.1f%%", v * 100);
            return std::string(buf);
        };
        char line[256];
        std::snprintf(line, sizeof(line), "%-9s %-5d %-10s %-9s %-9s %-11lld %-12d %-7s %-11lld %lld",
                      name.c_str(), static_cast<int>(rs.size()), rate("partner").c_str(), rate("steady").c_str(),
                      rate("beyond").c_str(), totalSum / static_cast<long long>(rs.size()), median,
                      ([&] { char b[16]; std::snprintf(b, sizeof(b), "%.1f%%",
                              static_cast<double>(failed) / rs.size() * 100); return std::string(b); })().c_str(),
                      studySum / static_cast<long long>(rs.size()),
                      minMoneySum / static_cast<long long>(rs.size()));
        std::cout << line << "\n";
    }
}
