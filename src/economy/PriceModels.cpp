#include "economy/PriceModels.h"

#include <algorithm>
#include <cmath>

double BasePriceModel::price(const PriceContext& c) const {
    double locFactor = 1.0;
    auto it = c.goods.locFactor.find(c.location.id);
    if (it != c.goods.locFactor.end()) locFactor = it->second;
    return static_cast<double>(c.goods.basePrice) * locFactor;
}

double SeasonDecorator::factor(const PriceContext& c) const {
    for (int w : c.goods.peakWeeks)
        if (w == c.week) return 1.0 + amp_;
    return 1.0;
}

double TrendDecorator::factor(const PriceContext& c) const {
    return 1.0 + c.trend;
}

double ShockDecorator::factor(const PriceContext& c) const {
    return c.shockMult;
}

double NoiseDecorator::factor(const PriceContext& c) const {
    return c.noiseMult;
}

double ClampDecorator::price(const PriceContext& c) const {
    double p = inner_->price(c);
    p = std::clamp(p, 1.0, 999999.0);
    return std::round(p);
}

std::unique_ptr<IPriceModel> buildDefaultPriceChain(const BalanceDef& b) {
    // 组装顺序：base → season → trend → shock → noise → clamp
    std::unique_ptr<IPriceModel> chain = std::make_unique<BasePriceModel>();
    chain = std::make_unique<SeasonDecorator>(std::move(chain), b.seasonAmp);
    chain = std::make_unique<TrendDecorator>(std::move(chain));
    chain = std::make_unique<ShockDecorator>(std::move(chain));
    chain = std::make_unique<NoiseDecorator>(std::move(chain));
    chain = std::make_unique<ClampDecorator>(std::move(chain));
    return chain;
}
