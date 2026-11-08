#pragma once
// ============================================================
// [stock/StockSystem] 校园炒股模拟器（组长 A；玩法灵感：bilibili 炒外汇模拟器）
// - 虚构标的「浦江文旅（SHU）」：行情以 K 线（OHLC 蜡烛）呈现，
//   每次行动 / 周日结算生成一根新 K 线（内部再走子步），StockScreen 渲染
// - 股力系统：玩家通过"学股市知识"（耗大量体力）提升股力，
//   股力越高 → 持仓时 K 线越偏向上涨；但永远保留亏钱可能（概率倾斜而非确定性）
// - 新手被收割：股力低时持仓下跌概率 stockFailProb（默认 62%）；
//   体力 < 阈值还硬炒，下跌概率再往上加
// - 杠杆融资（1x/2x/5x）：持仓净值 ≤ 0 强制平仓（爆仓），钱包可扣成负数；
//   融资负债按周计息；欠债超过 bankruptcyDebt → bankrupt flag → 《破产清算》结局
// - 买卖不耗行动点，但每次消耗体力（staminaStockCost）
// - 行情使用独立随机流（stockRng_）：不扰动主随机流，Bot 模拟校准不受影响
// ============================================================
#include <string>
#include <vector>

#include "core/ConfigDefs.h"
#include "core/ISaveable.h"
#include "core/Random.h"
#include "player/TradeSystem.h" // TradeResult

class ConfigManager;
class GameContext;

struct StockBar {
    double o = 0; // 开
    double h = 0; // 高
    double l = 0; // 低
    double c = 0; // 收
};

class StockSystem : public ISaveable {
public:
    StockSystem(const ConfigManager& cfg, Random& rng);

    // ---- 时间驱动（Session/WeekSettlement 调用） ----
    void onTick(GameContext& ctx);    // 生成一根新 K 线 + 爆仓检查
    long long chargeWeeklyInterest(); // 融资负债按周计息，返回利息额

    // ---- 外部冲击（事件效果/测试用）：价格 ×= mult ----
    void applyExternalShock(GameContext& ctx, double mult);

    // ---- 玩家操作（不耗行动点，耗体力） ----
    TradeResult buy(GameContext& ctx, int yuan); // 投入 yuan 现金按当前杠杆买入
    TradeResult sellAll(GameContext& ctx);       // 清仓
    TradeResult setLeverage(int lev);            // 空仓时切换杠杆

    // ---- 行情/持仓只读接口（UI 用） ----
    double price() const;                          // 现价 = 最新收盘价
    const std::vector<StockBar>& bars() const;     // K 线序列
    int shares() const;
    double avgCost() const;
    long long loan() const;
    int leverage() const;
    double positionValue() const;  // 持仓市值 = 股数 × 现价
    double equity() const;         // 净持仓 = 市值 - 融资负债
    double unrealizedPnl() const;  // 浮动盈亏（对总成本）
    const std::vector<int>& leverageOptions() const;
    double effectiveFailProb(const GameContext& ctx) const; // 持仓时下一根 K 线收阴概率
    const std::string& notice() const; // 爆仓等系统提示（UI 显示）

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    void step(GameContext& ctx, double close, const StockBar& bar); // 收出一根 K 线 + 记历史 + 爆仓检查
    void liquidate(GameContext& ctx);          // 强制平仓（可能产生债务）

    const ConfigManager& cfg_;
    Random& rng_;         // 仅用于构造时给行情流播种
    Random stockRng_;     // 行情独立随机流（不扰动主流 → Bot 模拟校准不变）
    double price_;        // 现价 = 最新收盘价
    std::vector<StockBar> bars_;
    int shares_ = 0;
    double costBasis_ = 0.0;      // 持仓总成本（买入价 × 股数，含融资部分）
    double principal_ = 0.0;      // 自有本金累计（实付现金 + 买入手续费）
    long long loan_ = 0;          // 融资负债（元）
    int leverageIndex_ = 0;
    std::string notice_;
};
