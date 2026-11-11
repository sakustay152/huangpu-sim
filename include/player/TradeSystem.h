#pragma once
// ============================================================
// [player/TradeSystem] 交易系统（组员 C 主责；A 集成最小版）
// 分工 7.4：买入/卖出校验（钱、容量、限购、地点），返回 TradeResult
// 说明失败原因，不用异常做流程控制。交易成功发布 TradeCompletedEvent。
// 社交线折扣也在这里结算（flag 驱动：学长资料 9 折、学姐内部票加成）。
// ============================================================
#include <string>

#include "core/GameContext.h"
#include "player/TradeError.h"

struct TradeResult {
    bool ok = false;
    std::string msg;
    int amount = 0; // 成交总额（正数）

    // ↓ 新增字段只能追加在最后：仓库里到处是 {false, "..."} / {true, msg, total}
    //   的聚合初始化，插到中间会让它们全部编译失败。
    TradeError code = TradeError::None;
    int unitPrice = 0; // 单价（失败时也给报价，方便 UI 提示差额）
    int maxQty = 0;    // 本次最多能成交多少（失败时给 UI "最多 X 件"）
};

class TradeSystem {
public:
    explicit TradeSystem(const ConfigManager& cfg) : cfg_(cfg) {}

    TradeResult buy(GameContext& ctx, const std::string& goodsId, int qty) const;
    TradeResult sell(GameContext& ctx, const std::string& goodsId, int qty) const;

    // 期末估值：背包按各地最高卖价估算（模拟器/结局判定/结局页共用）
    int estimatePortfolioValue(const GameContext& ctx) const;

private:
    // 统一的失败构造：业务分支只传 code/msg/maxQty/unitPrice
    TradeResult fail(TradeError c, const std::string& m,
                     int maxQty = 0, int unitPrice = 0) const;

    // v2 时间系统：成交一趟市场要花时间（买/卖各 tradeHours 小时）
    void passMarketHour(GameContext& ctx) const;

    const ConfigManager& cfg_;
};
