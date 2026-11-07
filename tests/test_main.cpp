// ============================================================
// [tests] 核心规则测试（组长 A；轻量断言框架，日后可平移到 Catch2）
// 覆盖分工 7.4 的单测点：钱守恒、容量、价格边界、存档往返、
// 结局判定、模拟器不变量（钱不为负 / 价格不爆）。
// 运行：huangpu_tests.exe [--data 目录]
// ============================================================
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "actions/Commands.h"
#include "core/ConfigManager.h"
#include "core/EventBus.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "core/Session.h"
#include "core/WeekSettlement.h"
#include "economy/Market.h"
#include "events/EndingJudge.h"
#include "events/GameEvent.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "simulate/SimulationRunner.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"

namespace {
int g_failed = 0;
int g_total = 0;

void check(bool cond, const std::string& what) {
    ++g_total;
    if (cond) {
        std::cout << "  [PASS] " << what << "\n";
    } else {
        ++g_failed;
        std::cout << "  [FAIL] " << what << "\n";
    }
}

std::string findDataDir() {
    namespace fs = std::filesystem;
    if (fs::exists("data/balance.json")) return "data";
    if (fs::exists("../data/balance.json")) return "../data";
    return "data";
}
} // namespace

int main(int argc, char** argv) {
    std::string dataDir = findDataDir();
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--data" && i + 1 < argc) dataDir = argv[++i];
    }
    std::cout << "测试数据目录: " << dataDir << "\n";

    try {
        ConfigManager cfg(dataDir);

        // ---------- 1. Wallet 封装：钱守恒 / 不为负 ----------
        std::cout << "\n== Wallet ==\n";
        Wallet w(100);
        check(w.spend(30) && w.get() == 70, "正常扣款 100-30=70");
        check(!w.spend(999) && w.get() == 70, "余额不足拒绝扣款");
        w.add(30);
        check(w.get() == 100, "收入入账");
        Json wj = w.toJson();
        Wallet w2;
        w2.fromJson(wj);
        check(w2.get() == 100, "钱包序列化往返");

        // ---------- 2. Inventory：容量 / 平均成本 ----------
        std::cout << "\n== Inventory ==\n";
        Inventory inv(10);
        check(inv.canFit(10, 1), "10 件 1 格商品可入");
        check(!inv.canFit(3, 4), "3 件 4 格大件超容拒绝");
        inv.add("umbrella", 4, 30.0, 1);
        inv.add("umbrella", 6, 40.0, 1);
        check(inv.find("umbrella")->qty == 10, "堆叠合并");
        check(std::abs(inv.find("umbrella")->avgCost - 36.0) < 1e-9, "加权均价 (4*30+6*40)/10=36");
        check(inv.usedSlots() == 10 && inv.freeSlots() == 0, "占格统计");
        check(inv.remove("umbrella", 12) == false, "超量出包拒绝");
        check(inv.remove("umbrella", 10), "全部出包");

        // ---------- 3. Random：同种子同序列 ----------
        std::cout << "\n== Random ==\n";
        Random r1(7), r2(7);
        bool same = true;
        for (int i = 0; i < 100; ++i)
            if (r1.range(1, 1000) != r2.range(1, 1000)) same = false;
        check(same, "同种子序列完全一致（bug 可复现）");

        // ---------- 4. Market：价格边界 ----------
        std::cout << "\n== Market ==\n";
        Random rng(1);
        EventBus bus;
        GameContext ctx(cfg, rng, bus);
        ctx.initNewGame();
        bool priceOk = true;
        for (const auto& l : cfg.locations())
            for (const auto& g : cfg.goods()) {
                if (ctx.market->buyPrice(l.id, g.id) < 1 || ctx.market->sellPrice(l.id, g.id) < 1)
                    priceOk = false;
                if (ctx.market->sellPrice(l.id, g.id) > ctx.market->buyPrice(l.id, g.id))
                    priceOk = false; // 卖价 ≤ 买价（价差保护）
            }
        check(priceOk, "全部 (6 地点 x 12 商品) 价格 ≥1 且 卖价≤买价");
        ctx.market->applyShock("umbrella", 1.8, 2);
        int shockPrice = ctx.market->buyPrice("store", "umbrella");
        check(shockPrice > 40, "暴雨冲击后雨伞买价暴涨（>40）");

        // ---------- 5. 交易：守恒 / 校验 ----------
        std::cout << "\n== TradeSystem ==\n";
        ctx.locationId = "store";
        int before = ctx.player->wallet().get();
        auto rb = ctx.trade->buy(ctx, "coffee_ticket", 5);
        check(rb.ok, "教育超市买入咖啡券 x5");
        int cost = before - ctx.player->wallet().get();
        check(cost == 5 * ctx.market->buyPrice("store", "coffee_ticket"), "支出 = 单价x数量（钱守恒）");
        check(!ctx.trade->buy(ctx, "show_ticket", 1).ok, "非本地点购买被拒绝");
        auto rs = ctx.trade->sell(ctx, "coffee_ticket", 5);
        check(rs.ok && ctx.player->wallet().get() < before, "卖出回款但低于成本（价差存在）");
        check(ctx.player->inventory().find("coffee_ticket") == nullptr, "卖光后背包清空");

        // ---------- 6. 行动点与结算 ----------
        std::cout << "\n== TimeSystem / WeekSettlement ==\n";
        check(ctx.time->actionsLeft() == cfg.balance().apPerWeek, "周初行动点满额");
        ctx.locationId = "canteen";
        while (ctx.time->canAct()) {
            Session::runAction(ctx, WorkCommand());
        }
        check(ctx.time->actionsLeft() == 0, "行动点耗尽");
        int walletBefore = ctx.player->wallet().get();
        WeekSettlement settle(ctx);
        auto rep = settle.run();
        check(ctx.time->week() == 2, "结算后进入第 2 周");
        check(ctx.player->wallet().get() <= walletBefore - cfg.balance().weeklyExpense + 1,
              "周支出已扣除");

        // ---------- 7. 存档往返 ----------
        std::cout << "\n== 存档往返 ==\n";
        ctx.player->wallet().add(500);
        ctx.npcs->addAffinity("roommate", 25);
        Json save = ctx.toJson();
        GameContext ctx2(cfg, rng, bus);
        ctx2.initNewGame();
        ctx2.loadFromJson(save);
        check(ctx2.player->wallet().get() == ctx.player->wallet().get(), "现金一致");
        check(ctx2.time->week() == ctx.time->week(), "周数一致");
        check(ctx2.npcs->affinity("roommate") == 25, "好感一致");
        check(ctx2.player->inventory().capacity() == cfg.balance().maxSlots, "背包容量一致");

        // ---------- 8. 结局判定 ----------
        std::cout << "\n== EndingJudge ==\n";
        GameContext e1(cfg, rng, bus);
        e1.initNewGame();
        check(std::string(EndingJudge::judge(e1)->id) == "beyond", "初始状态 → 账单之外（兜底）");
        e1.player->wallet().add(20000);
        e1.npcs->addAffinity("roommate", 70);
        check(std::string(EndingJudge::judge(e1)->id) == "steady", "达标+挚友 → 站稳脚跟");
        e1.npcs->addAffinity("xuejie", 85);
        e1.flags->set("club_member");
        check(std::string(EndingJudge::judge(e1)->id) == "partner", "社团线+学姐 85 → 校园合伙人");

        // ---------- 9. 模拟器不变量（抽样 30 局） ----------
        std::cout << "\n== Simulation 不变量 ==\n";
        SimulationRunner runner(cfg);
        auto results = runner.runGames("mixed", 12345, 30);
        bool noCrash = results.size() == 30;
        bool moneyOk = true, endingOk = true;
        for (const auto& r : results) {
            if (r.minMoney < 0) moneyOk = false;
            if (r.endingId.empty()) endingOk = false;
        }
        check(noCrash, "30 局 headless 无崩溃");
        check(moneyOk, "全程现金 ≥ 0（钱不为负不变量）");
        check(endingOk, "每局都能得到结局");

    } catch (const std::exception& e) {
        std::cout << "[测试环境异常] " << e.what() << "\n";
        return 2;
    }

    std::cout << "\n==== 测试结果: " << (g_total - g_failed) << "/" << g_total << " 通过 ====\n";
    return g_failed == 0 ? 0 : 1;
}
