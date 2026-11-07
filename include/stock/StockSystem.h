#pragma once
// ============================================================
// [stock/StockSystem] 校园炒股模拟器（组长 A；玩法灵感：bilibili 炒外汇模拟器）
// - 虚构标的「浦江文旅（SHU）」：价格随机走，每次行动 / 周日结算波动一格，
//   ASCII 走势图在 StockScreen 渲染（对应 fx-simple 的折线图）
// - 新手被收割：持仓时下跌概率 stockFailProb（默认 62%）显著高于上涨；
//   体力 < 阈值还硬炒，下跌概率再往上加
// - 杠杆融资（1x/2x/5x）：持仓净值 ≤ 0 强制平仓（爆仓），钱包可扣成负数；
//   融资负债按周计息；欠债超过 bankruptcyDebt → bankrupt flag → 《破产清算》结局
// - 买卖不耗行动点，但每次消耗体力（staminaStockCost）
// ============================================================
#include <string>
#include <vector>

#include "core/ConfigDefs.h"
#include "core/ISaveable.h"
#include "player/TradeSystem.h" // TradeResult

class ConfigManager;
class Random;
class GameContext;

class StockSystem : public ISaveable {
public:
    StockSystem(const ConfigManager& cfg, Random& rng);

    // ---- 时间驱动（ITimeListener 语义，Session/WeekSettlement 调用） ----
    void onTick(GameContext& ctx); // 价格随机走一格 + 爆仓检查
    long long chargeWeeklyInterest(); // 融资负债按周计息，返回利息额

    // ---- 外部冲击（事件效果/测试用）：价格 ×= mult ----
    void applyExternalShock(GameContext& ctx, double mult);

    // ---- 玩家操作（不耗行动点，耗体力） ----
    TradeResult buy(GameContext& ctx, int yuan); // 投入 yuan 现金按当前杠杆买入
    TradeResult sellAll(GameContext& ctx);       // 清仓
    TradeResult setLeverage(int lev);            // 空仓时切换杠杆

    // ---- 行情/持仓只读接口（UI 用） ----
    double price() const;
    const std::vector<double>& history() const;
    int shares() const;
    double avgCost() const;
    long long loan() const;
    int leverage() const;
    double positionValue() const;  // 持仓市值 = 股数 × 现价
    double equity() const;         // 净持仓 = 市值 - 融资负债
    double unrealizedPnl() const;  // 浮动盈亏（对总成本）
    const std::vector<int>& leverageOptions() const;
    double effectiveFailProb(const GameContext& ctx) const; // 含低体力惩罚的下跌概率
    const std::string& notice() const; // 爆仓等系统提示（UI 显示）

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    void step(GameContext& ctx, double drift); // 走一格 + 记历史 + 爆仓检查
    void liquidate(GameContext& ctx);          // 强制平仓（可能产生债务）

    const ConfigManager& cfg_;
    Random& rng_;
    double price_;
    std::vector<double> history_; // 近 120 格走势（UI 画图）
    int shares_ = 0;
    double costBasis_ = 0.0;      // 持仓总成本（买入价 × 股数，含融资部分）
    double principal_ = 0.0;      // 自有本金累计（实付现金 + 买入手续费）
    long long loan_ = 0;          // 融资负债（元）
    int leverageIndex_ = 0;
    std::string notice_;
};
