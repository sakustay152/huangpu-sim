#pragma once
// ============================================================
// [economy/PriceModels] 价格因子链（B 主责的接口已有；A 集成参考实现）
// 设计（分工 9）：Strategy + Decorator ——
//   BasePriceModel 是被装饰的底层；Season/Trend/Shock/Noise 是装饰器，
//   逐层相乘；ClampDecorator 兜底钳制。要加新因子=新增一个类。
//   为什么不用一个巨型公式：因子可独立单测、可自由组合、可按商品开关。
// ============================================================
#include <memory>
#include <vector>

#include "core/ConfigDefs.h"
#include "core/IPriceModel.h"

// 底层：base × 地点系数
class BasePriceModel : public IPriceModel {
public:
    double price(const PriceContext& c) const override;
};

// 装饰器基类：内层结果 × 本层因子
class PriceDecorator : public IPriceModel {
public:
    explicit PriceDecorator(std::unique_ptr<IPriceModel> inner) : inner_(std::move(inner)) {}
    double price(const PriceContext& c) const override { return inner_->price(c) * factor(c); }

protected:
    virtual double factor(const PriceContext& c) const = 0;
    std::unique_ptr<IPriceModel> inner_;
};

// 需求季节：命中 peakWeeks 时价格上浮 seasonAmp
class SeasonDecorator : public PriceDecorator {
public:
    explicit SeasonDecorator(std::unique_ptr<IPriceModel> inner, double amp)
        : PriceDecorator(std::move(inner)), amp_(amp) {}
protected:
    double factor(const PriceContext& c) const override;

private:
    double amp_;
};

// 周趋势：本周内的整体涨跌
class TrendDecorator : public PriceDecorator {
public:
    explicit TrendDecorator(std::unique_ptr<IPriceModel> inner) : PriceDecorator(std::move(inner)) {}
protected:
    double factor(const PriceContext& c) const override;
};

// 事件冲击：双十一/暴雨等造成的短时暴涨暴跌
class ShockDecorator : public PriceDecorator {
public:
    explicit ShockDecorator(std::unique_ptr<IPriceModel> inner) : PriceDecorator(std::move(inner)) {}
protected:
    double factor(const PriceContext& c) const override;
};

// 日内噪声：每次行动后的小幅波动
class NoiseDecorator : public PriceDecorator {
public:
    explicit NoiseDecorator(std::unique_ptr<IPriceModel> inner) : PriceDecorator(std::move(inner)) {}
protected:
    double factor(const PriceContext& c) const override;
};

// 兜底：钳制到 [1, 999999] 并取整，防负价/爆炸（分工 3.4 价格规则）
class ClampDecorator : public PriceDecorator {
public:
    explicit ClampDecorator(std::unique_ptr<IPriceModel> inner) : PriceDecorator(std::move(inner)) {}
    double price(const PriceContext& c) const override;

protected:
    double factor(const PriceContext&) const override { return 1.0; } // price 已整体覆写
};

// 工厂：组装默认价格链（Factory 模式的落点之一）
std::unique_ptr<IPriceModel> buildDefaultPriceChain(const BalanceDef& b);
