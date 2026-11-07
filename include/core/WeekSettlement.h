#pragma once
// ============================================================
// [core/WeekSettlement] 周日结算（组长 A 维护）
// Template Method 模式（分工 9）：结算的"步骤与顺序"固定在 run()，
// 每一步是受保护的虚函数 —— B/C/D 以后重写某一步（如精力结算）
// 不需要动主流程，顺序也不会被改乱。
// ============================================================
#include <vector>

#include "util/Json.h"

class GameContext;

class WeekSettlement {
public:
    struct Report {
        int week = 0;
        int moneyBefore = 0;
        int moneyAfter = 0;
        bool semesterOver = false;
        std::vector<std::string> lines;
    };

    explicit WeekSettlement(GameContext& ctx) : ctx_(ctx) {}
    virtual ~WeekSettlement() = default;

    // 模板方法：固定顺序（支出→学业→延迟效果→节点→进入新周→行情/情报）
    Report run();

protected:
    GameContext& ctx_;
    Report rep_;

    virtual void chargeExpenses();        // 每周生活费支出
    virtual void settleStudy();           // 一周没学习扣平时分
    virtual void applyScheduledEffects(); // 延迟效果（如室友还钱）
    virtual void settleStocks();          // 股票融资利息 + 周结算时股价走一格
    virtual void settleEnergy();          // 每周体力回血
    virtual void checkMilestones();       // 期中警告 / 期末判定 / 奖学金 / 补考费
    virtual void beginNewWeek();          // 周数+1、冷却递减、广播 WeekStarted
    virtual void refreshMarketAndRumors(); // 行情周刷新 + 新情报
};
