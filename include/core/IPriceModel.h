#pragma once
// ============================================================
// [core/IPriceModel] 价格模型接口（组长 A 维护，W1 冻结接口；B 实现）
// 分工 7.3：mid = base × location × season × trend × noise × shock
// 用"策略 + 装饰器"叠加价格因子，避免一个巨型公式：
//   BasePriceModel → SeasonDecorator → TrendDecorator
//                 → ShockDecorator → NoiseDecorator → ClampDecorator
// 每个因子一个类，新增因子不改旧代码（开闭原则）。
// ============================================================
#include "core/ConfigDefs.h"

struct PriceContext {
    const GoodsDef& goods;       // 商品定义
    const LocationDef& location; // 所在地点
    int week = 1;                // 学期第几周
    double trend = 0.0;          // 本周趋势（-trendAmp ~ +trendAmp）
    double noiseMult = 1.0;      // 当日噪声乘数（≈1）
    double shockMult = 1.0;      // 事件冲击乘数（≈1）
};

class IPriceModel {
public:
    virtual ~IPriceModel() = default;

    // 返回该商品在该地点、该上下文下的"中间价"（未加价差）
    virtual double price(const PriceContext& c) const = 0;
};
