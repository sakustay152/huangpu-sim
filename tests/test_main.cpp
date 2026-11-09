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
#include "player/TradeError.h"
#include "player/TradeSystem.h"
#include "simulate/SimulationRunner.h"
#include "stock/StockSystem.h"
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

        // ---------- 10. 炒股：体力 / 杠杆 / 爆仓 / 破产 ----------
        std::cout << "\n== StockSystem / Energy / 破产 ==\n";
        {
            GameContext sctx(cfg, rng, bus);
            sctx.initNewGame();

            // 体力
            EnergySystem& en = sctx.player->energy();
            check(en.get() == cfg.balance().staminaMax, "体力初始满值");
            check(!en.spend(9999), "体力不足拒绝消耗");
            check(en.spend(cfg.balance().staminaStockCost), "正常消耗体力");
            check(en.get() == cfg.balance().staminaMax - cfg.balance().staminaStockCost,
                  "体力扣减正确");
            en.add(cfg.balance().staminaRestGain);
            check(en.get() == std::min(cfg.balance().staminaMax,
                                       cfg.balance().staminaMax - cfg.balance().staminaStockCost +
                                           cfg.balance().staminaRestGain),
                  "休息恢复体力（不超上限）");

            // Wallet：唯一允许负债的入口
            sctx.player->wallet().charge(cfg.balance().startMoney + 10);
            check(sctx.player->wallet().get() == -10, "charge 可扣成负数（债务）");
            sctx.player->wallet().add(cfg.balance().startMoney); // 回血，继续后续用例

            // 价格随机走：同种子完全一致（bug 可复现），且始终有界
            Random rw1(9), rw2(9);
            StockSystem st1(cfg, rw1), st2(cfg, rw2);
            for (int i = 0; i < 20; ++i) {
                st1.onTick(sctx);
                st2.onTick(sctx);
            }
            check(std::abs(st1.price() - st2.price()) < 1e-9, "同种子股价序列完全一致");
            check(st1.price() >= 0.5 && st1.price() <= 999.0, "股价始终在 [0.5, 999] 内");

            // 杠杆买入：x5 投入 2900 → 仓位 14500
            sctx.stock->setLeverage(5);
            auto rb = sctx.stock->buy(sctx, 2900);
            check(rb.ok, "杠杆 x5 买入 2900 元");
            double p0 = sctx.stock->price();
            int expectShares = static_cast<int>(std::floor(2900.0 * 5 / p0));
            int expectCost = static_cast<int>(std::llround(expectShares * p0));
            check(sctx.stock->shares() == expectShares, "股数 = 投入×杠杆/现价");
            check(sctx.stock->loan() == expectCost - 2900, "融资负债 = 仓位 - 自有资金");

            // 爆仓：价格 ×0.25 → 净值 ≤ 0 → 强制平仓 → 负债 → 破产 flag
            sctx.stock->applyExternalShock(sctx, 0.25);
            check(sctx.stock->shares() == 0, "净值 ≤ 0 强制平仓");
            check(sctx.player->wallet().get() < 0, "爆仓后钱包为负（债务）");
            check(sctx.player->wallet().get() < -cfg.balance().bankruptcyDebt, "欠债超过破产线");
            check(sctx.flags->has("bankrupt"), "爆仓欠债 → 破产 flag");

            // 存档往返：股价/持仓/负债/体力
            Json sj = sctx.stock->toJson();
            GameContext lctx(cfg, rng, bus);
            lctx.initNewGame();
            lctx.stock->fromJson(sj);
            check(std::abs(lctx.stock->price() - sctx.stock->price()) < 1e-9, "股价存档往返");
            check(lctx.stock->loan() == sctx.stock->loan(), "融资负债存档往返");
            Json pj = sctx.player->toJson();
            lctx.player->fromJson(pj);
            check(lctx.player->energy().get() == sctx.player->energy().get(), "体力存档往返");
            check(lctx.player->stockSkill() == sctx.player->stockSkill(), "股力存档往返");
        }

        // ---------- 11. K 线形态 / 股力 / 学股市知识 ----------
        std::cout << "\n== K线 / 股力 ==\n";
        {
            GameContext kctx(cfg, rng, bus);
            kctx.initNewGame();

            // onTick 生成一根形态合法的 K 线
            std::size_t n0 = kctx.stock->bars().size();
            kctx.stock->onTick(kctx);
            check(kctx.stock->bars().size() == n0 + 1, "onTick 生成一根新 K 线");
            const StockBar& b = kctx.stock->bars().back();
            check(b.h >= b.l && b.c <= b.h && b.c >= b.l && b.o <= b.h && b.o >= b.l,
                  "K 线 OHLC 形态合法（高≥开收低≥低）");

            // 学股市知识：耗大量体力 + 行动点，提升股力
            EnergySystem& ken = kctx.player->energy();
            int staminaBefore = ken.get();
            int skill0 = kctx.player->stockSkill();
            auto out = Session::runAction(kctx, LearnStockCommand());
            check(out.result.ok && out.result.consumedAp, "学股市知识消耗 1 行动点");
            check(ken.get() == staminaBefore - cfg.balance().learnStockStaminaCost,
                  "学股市知识消耗大量体力");
            check(kctx.player->stockSkill() >= skill0 + cfg.balance().learnStockGainMin &&
                      kctx.player->stockSkill() <= skill0 + cfg.balance().learnStockGainMax,
                  "股力在增益区间内提升");

            // 股力越高 → 收阴概率越低（但有下限，永不稳赚）
            double pLow = kctx.stock->effectiveFailProb(kctx);
            kctx.player->addStockSkill(cfg.balance().stockSkillMax); // 拉满
            double pMax = kctx.stock->effectiveFailProb(kctx);
            check(pMax < pLow, "股力越高 K 线越偏向赚钱方向");
            check(pMax >= cfg.balance().stockSkillFailFloor - 1e-9, "满股力仍有最低收阴概率（可能亏钱）");
            double expected = std::max(cfg.balance().stockFailProb - kctx.player->stockSkill() * cfg.balance().stockSkillFactor,
                                       cfg.balance().stockSkillFailFloor);
            check(std::abs(pMax - expected) < 1e-9, "收阴概率 = 基准 - 股力修正（下限封顶）");
        }

        // ---------- 12. C 模块：交易错误码（W2） ----------
        std::cout << "\n== C 模块：交易错误码 ==\n";
        {
            GameContext t(cfg, rng, bus);
            t.initNewGame();
            t.locationId = "store";

            const int cash0 = t.player->wallet().get();

            // 容量：咖啡券 1 格/件，背包 30 格 → 最多 30
            auto r1 = t.trade->buy(t, "coffee_ticket", 100);
            check(!r1.ok && r1.code == TradeError::NotEnoughSlot,
                  "买 100 件超容量 → NotEnoughSlot");
            check(r1.maxQty == 30, "超容量时 maxQty=30");
            check(t.player->wallet().get() == cash0, "失败后现金不变");
            check(t.player->inventory().empty(), "失败后背包不变");

            // 限购：充电宝 15/天，容量 30 不构成瓶颈
            auto r2 = t.trade->buy(t, "powerbank", 16);
            check(!r2.ok && r2.code == TradeError::QuotaExceeded,
                  "超限购 → QuotaExceeded");
            check(r2.maxQty == 15, "超限购时 maxQty=15");

            // 现金：只剩 50 元，咖啡券单价约 10 元
            t.player->wallet().spend(t.player->wallet().get() - 50);
            auto r3 = t.trade->buy(t, "coffee_ticket", 10);
            check(!r3.ok && r3.code == TradeError::NotEnoughCash,
                  "现金不足 → NotEnoughCash");
            check(r3.maxQty > 0 && r3.maxQty <= 5, "现金不足时 maxQty=可买件数");

            // 数量非法
            auto r4 = t.trade->buy(t, "coffee_ticket", 0);
            check(!r4.ok && r4.code == TradeError::InvalidCount, "数量 0 → InvalidCount");

            // 地点不卖
            t.locationId = "library";
            auto r5 = t.trade->buy(t, "coffee_ticket", 1);
            check(!r5.ok && r5.code == TradeError::WrongLocation,
                  "图书馆买咖啡券 → WrongLocation");

            // 成功路径：金额 = 单价 × 数量
            t.locationId = "store";
            t.player->wallet().add(500);
            auto ok = t.trade->buy(t, "coffee_ticket", 5);
            check(ok.ok && ok.code == TradeError::None, "正常买入成功");
            check(ok.amount == ok.unitPrice * 5, "amount = unitPrice × qty");

            // 卖出超量
            auto over = t.trade->sell(t, "coffee_ticket", 8);
            check(!over.ok && over.code == TradeError::NotOwned, "卖出超量 → NotOwned");
            check(t.player->inventory().find("coffee_ticket")->qty == 5,
                  "失败后仍持有 5 件");
        }

    } catch (const std::exception& e) {
        std::cout << "[测试环境异常] " << e.what() << "\n";
        return 2;
    }

    std::cout << "\n==== 测试结果: " << (g_total - g_failed) << "/" << g_total << " 通过 ====\n";
    return g_failed == 0 ? 0 : 1;
}
