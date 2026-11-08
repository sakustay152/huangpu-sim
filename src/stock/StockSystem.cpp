#include "stock/StockSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "core/Random.h"
#include "player/Player.h"
#include "story/StoryFlags.h"
#include "util/JsonUtil.h"

namespace {
std::string fmtPrice(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}
} // namespace

StockSystem::StockSystem(const ConfigManager& cfg, Random& rng)
    : cfg_(cfg),
      rng_(rng),
      stockRng_(rng.next()), // 行情独立随机流：一次播种，不扰动主流
      price_(cfg.balance().stockStartPrice) {
    StockBar first;
    first.o = first.h = first.l = first.c = price_;
    bars_.push_back(first);
}

double StockSystem::price() const { return price_; }
const std::vector<StockBar>& StockSystem::bars() const { return bars_; }
int StockSystem::shares() const { return shares_; }
double StockSystem::avgCost() const { return shares_ > 0 ? costBasis_ / shares_ : 0.0; }
long long StockSystem::loan() const { return loan_; }
int StockSystem::leverage() const { return cfg_.balance().stockLeverages[leverageIndex_]; }
double StockSystem::positionValue() const { return shares_ * price_; }
double StockSystem::equity() const { return positionValue() - static_cast<double>(loan_); }
double StockSystem::unrealizedPnl() const { return positionValue() - costBasis_; }
const std::vector<int>& StockSystem::leverageOptions() const { return cfg_.balance().stockLeverages; }
const std::string& StockSystem::notice() const { return notice_; }

double StockSystem::effectiveFailProb(const GameContext& ctx) const {
    const BalanceDef& b = cfg_.balance();
    // 股力越高，持仓时下一根 K 线收阴概率越低（但永远保底 25%，不会稳赚）
    double p = b.stockFailProb - ctx.player->stockSkill() * b.stockSkillFactor;
    p = std::max(p, b.stockSkillFailFloor);
    if (ctx.player->energy().isLow()) p += b.staminaLowFailBonus;
    return std::min(p, 0.95);
}

void StockSystem::onTick(GameContext& ctx) {
    const BalanceDef& b = cfg_.balance();
    bool holding = shares_ > 0;
    double failP = holding ? effectiveFailProb(ctx) : 0.5;

    // 一根 K 线：开盘 = 前收盘，内部再走若干子步形成高低价，收盘决定下一根开盘
    StockBar bar;
    bar.o = price_;
    bar.h = price_;
    bar.l = price_;
    double p = price_;
    for (int i = 0; i < b.stockIntraSteps; ++i) {
        double drift;
        if (holding) {
            // 大一学生是被收割对象：股力低时 K 线更倾向收阴
            drift = stockRng_.chance(failP)
                        ? -stockRng_.uniform(b.stockVolMin, b.stockVolMax)
                        : +stockRng_.uniform(b.stockVolMin, b.stockVolMax);
        } else {
            drift = (stockRng_.chance(0.5) ? 1.0 : -1.0) *
                    stockRng_.uniform(b.stockVolMin * 0.4, b.stockVolMax * 0.5);
        }
        p = std::clamp(p * (1.0 + drift), 0.5, 999.0);
        bar.h = std::max(bar.h, p);
        bar.l = std::min(bar.l, p);
    }
    if (stockRng_.chance(b.stockCrashProb)) { // 黑天鹅
        p = std::clamp(p * (1.0 - b.stockCrashDrop), 0.5, 999.0);
        bar.h = std::max(bar.h, p);
        bar.l = std::min(bar.l, p);
    }
    if (stockRng_.chance(b.stockBoomProb)) { // 天降横财
        p = std::clamp(p * (1.0 + b.stockBoomRise), 0.5, 999.0);
        bar.h = std::max(bar.h, p);
        bar.l = std::min(bar.l, p);
    }
    step(ctx, p, bar);
}

void StockSystem::applyExternalShock(GameContext& ctx, double mult) {
    if (mult <= 0.0) mult = 0.1;
    double target = std::clamp(price_ * mult, 0.5, 999.0);
    StockBar bar;
    bar.o = price_;
    bar.h = std::max(price_, target);
    bar.l = std::min(price_, target);
    bar.c = target;
    step(ctx, target, bar);
}

void StockSystem::step(GameContext& ctx, double close, const StockBar& bar) {
    notice_.clear();
    price_ = std::round(close * 100.0) / 100.0;
    StockBar fixed = bar;
    fixed.c = price_;
    fixed.h = std::round(std::max(fixed.h, price_) * 100.0) / 100.0;
    fixed.l = std::round(std::min(fixed.l, price_) * 100.0) / 100.0;
    bars_.push_back(fixed);
    if (bars_.size() > static_cast<std::size_t>(cfg_.balance().stockBarsKeep))
        bars_.erase(bars_.begin());
    if (shares_ > 0 && equity() <= 0.0) liquidate(ctx); // 爆仓
    ctx.updateBankruptcyFlag();
}

void StockSystem::liquidate(GameContext& ctx) {
    int proceeds = static_cast<int>(std::llround(positionValue()));
    long long debt = loan_;
    ctx.player->wallet().add(proceeds);
    ctx.player->wallet().charge(static_cast<int>(debt)); // 可扣成负数 → 债务
    notice_ = "【爆仓】持仓净值 ≤ 0，强制平仓！回款 ¥" + std::to_string(proceeds) +
              "，偿还融资 ¥" + std::to_string(debt) +
              "（自有本金 ¥" + std::to_string(static_cast<long long>(std::llround(principal_))) +
              " 全部损失）";
    shares_ = 0;
    costBasis_ = 0.0;
    principal_ = 0.0;
    loan_ = 0;
}
TradeResult StockSystem::buy(GameContext& ctx, int yuan) {
    const BalanceDef& b = cfg_.balance();
    if (yuan <= 0) return {false, "投入金额必须为正", 0};
    if (!ctx.player->energy().spend(b.staminaStockCost))
        return {false, "体力不支，盯不动盘了（需要 " + std::to_string(b.staminaStockCost) +
                           " 点体力，回宿舍休息恢复）", 0};

    int lev = leverage();
    int sharesNew = static_cast<int>(std::floor(static_cast<double>(yuan) * lev / price_));
    if (sharesNew <= 0)
        return {false, "金额太小，买不起一手（现价 ¥" + fmtPrice(price_) + "）", 0};

    int cost = static_cast<int>(std::llround(sharesNew * price_));
    int borrowed = std::max(0, cost - yuan); // 融资部分
    int fee = static_cast<int>(std::llround(cost * b.stockFeeRate));
    int outlay = std::min(cost, yuan); // 自有资金实付
    if (!ctx.player->wallet().canAfford(outlay + fee))
        return {false, "现金不够（还要留 ¥" + std::to_string(fee) + " 手续费）", 0};

    ctx.player->wallet().spend(outlay + fee);
    loan_ += borrowed;
    costBasis_ += cost;
    principal_ += outlay + fee; // 自有本金：实付现金 + 买入手续费
    shares_ += sharesNew;
    ctx.updateBankruptcyFlag();
    return {true, "买入 SHU x" + std::to_string(sharesNew) + "（杠杆 x" + std::to_string(lev) +
                      "）实付 ¥" + std::to_string(outlay + fee) +
                      (borrowed > 0 ? "，融资 ¥" + std::to_string(borrowed) : ""), outlay};
}

TradeResult StockSystem::sellAll(GameContext& ctx) {
    if (shares_ <= 0) return {false, "空仓，没什么可卖的", 0};
    const BalanceDef& b = cfg_.balance();
    if (!ctx.player->energy().spend(b.staminaStockCost))
        return {false, "体力不支…（需要 " + std::to_string(b.staminaStockCost) + " 点体力）", 0};

    int heldShares = shares_;
    double proceedsD = positionValue();
    int fee = static_cast<int>(std::llround(proceedsD * b.stockFeeRate));
    int proceeds = std::max(0, static_cast<int>(std::llround(proceedsD)) - fee);
    long long debt = loan_;
    long long principal = static_cast<long long>(std::llround(principal_));

    ctx.player->wallet().add(proceeds);
    if (debt > 0) ctx.player->wallet().charge(static_cast<int>(debt));
    long long net = static_cast<long long>(proceeds) - debt;
    long long pnl = net - principal; // 对自有资金的实现盈亏（含两笔手续费/利息）

    shares_ = 0;
    costBasis_ = 0.0;
    principal_ = 0.0;
    loan_ = 0;
    ctx.updateBankruptcyFlag();

    std::string msg = "清仓 SHU x" + std::to_string(heldShares) + "，回款 ¥" +
                      std::to_string(proceeds) +
                      (debt > 0 ? "，偿还融资 ¥" + std::to_string(debt) : "") +
                      "，净入 ¥" + std::to_string(net);
    if (pnl >= 0) msg += "（盈利 ¥" + std::to_string(pnl) + "）";
    else msg += "（亏损 ¥" + std::to_string(-pnl) + "）";
    return {true, msg, proceeds};
}

TradeResult StockSystem::setLeverage(int lev) {
    const auto& opts = cfg_.balance().stockLeverages;
    auto it = std::find(opts.begin(), opts.end(), lev);
    if (it == opts.end()) return {false, "不支持的杠杆倍数", 0};
    if (shares_ > 0) return {false, "有持仓不能改杠杆，先清仓", 0};
    leverageIndex_ = static_cast<int>(it - opts.begin());
    return {true, "杠杆切换为 x" + std::to_string(lev) + "（融资炒股，爆仓自负）", 0};
}

long long StockSystem::chargeWeeklyInterest() {
    if (loan_ <= 0) return 0;
    long long interest = static_cast<long long>(
        std::llround(loan_ * cfg_.balance().stockLoanWeeklyRate));
    if (interest <= 0) interest = 1;
    loan_ += interest;
    return interest;
}

Json StockSystem::toJson() const {
    Json bars = Json::array();
    for (const auto& b : bars_)
        bars.push_back(Json{{"o", b.o}, {"h", b.h}, {"l", b.l}, {"c", b.c}});
    return Json{
        {"price", price_},
        {"bars", bars},
        {"shares", shares_},
        {"costBasis", costBasis_},
        {"principal", principal_},
        {"loan", loan_},
        {"leverageIndex", leverageIndex_},
        {"rng", stockRng_.toJson()}, // 行情独立随机流状态随存档走
    };
}

void StockSystem::fromJson(const Json& j) {
    price_ = ju::getOr(j, "price", price_);
    bars_.clear();
    if (j.contains("bars") && j["bars"].is_array()) {
        for (const auto& e : j["bars"]) {
            StockBar b;
            b.o = ju::getOr(e, "o", price_);
            b.h = ju::getOr(e, "h", price_);
            b.l = ju::getOr(e, "l", price_);
            b.c = ju::getOr(e, "c", price_);
            if (!bars_.empty()) b.o = bars_.back().c; // 开盘衔接前一根收盘
            bars_.push_back(b);
        }
    } else if (j.contains("history") && j["history"].is_array()) {
        // 旧版存档（纯收盘价序列）→ 合成为平底 K 线，兼容读档
        for (const auto& p : j["history"]) {
            StockBar b;
            double c = p.get<double>();
            b.o = bars_.empty() ? c : bars_.back().c;
            b.h = std::max(b.o, c);
            b.l = std::min(b.o, c);
            b.c = c;
            bars_.push_back(b);
        }
    }
    if (bars_.empty()) {
        StockBar b;
        b.o = b.h = b.l = b.c = price_;
        bars_.push_back(b);
    }
    price_ = bars_.back().c;
    shares_ = ju::getOr(j, "shares", shares_);
    costBasis_ = ju::getOr(j, "costBasis", costBasis_);
    principal_ = ju::getOr(j, "principal", principal_);
    loan_ = ju::getOr(j, "loan", loan_);
    leverageIndex_ = ju::getOr(j, "leverageIndex", leverageIndex_);
    if (leverageIndex_ < 0 ||
        leverageIndex_ >= static_cast<int>(cfg_.balance().stockLeverages.size()))
        leverageIndex_ = 0;
    if (shares_ < 0) shares_ = 0;
    if (loan_ < 0) loan_ = 0;
    if (costBasis_ < 0) costBasis_ = 0.0;
    if (principal_ < 0) principal_ = 0.0;
    if (j.contains("rng")) stockRng_.fromJson(j["rng"]);
}
