#pragma once
// ============================================================
// [player/TradeSystem] 交易系统（组员 C 主责；A 集成最小版）
// 分工 7.4：买入/卖出校验（钱、容量、限购、地点），返回 TradeResult
// 说明失败原因，不用异常做流程控制。交易成功发布 TradeCompletedEvent。
// 社交线折扣也在这里结算（flag 驱动：学长资料 9 折、学姐内部票加成）。
// ============================================================
#include <string>

#include "core/GameContext.h"

struct TradeResult {
    bool ok = false;
    std::string msg;
    int amount = 0; // 成交总额（正数）
};

class TradeSystem {
public:
    explicit TradeSystem(const ConfigManager& cfg) : cfg_(cfg) {}

    TradeResult buy(GameContext& ctx, const std::string& goodsId, int qty) const;
    TradeResult sell(GameContext& ctx, const std::string& goodsId, int qty) const;

    // 期末估值：背包按各地最高卖价估算（模拟器/结局判定/结局页共用）
    int estimatePortfolioValue(const GameContext& ctx) const;

private:
    const ConfigManager& cfg_;
};
