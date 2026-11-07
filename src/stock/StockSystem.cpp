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
      price_(cfg.balance().stockStartPrice),
      history_{cfg.balance().stockStartPrice} {}

double StockSystem::price() const { return price_; }
const std::vector<double>& StockSystem::history() const { return history_; }
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
    double p = cfg_.balance().stockFailProb;
    if (ctx.player->energy().isLow()) p += cfg_.balance().staminaLowFailBonus;
    return std::min(p, 0.95);
}

void StockSystem::onTick(GameContext& ctx) {
    const BalanceDef& b = cfg_.balance();
    double drift = 0.0;
    if (shares_ > 0) {
        // 大一学生是被收割对象：持仓时下跌概率明显高于上涨（且越累越惨）
        drift = rng_.chance(effectiveFailProb(ctx))
                    ? -rng_.uniform(b.stockVolMin, b.stockVolMax)
                    : +rng_.uniform(b.stockVolMin, b.stockVolMax);
    } else {
        // 空仓看戏时市场相对中性
        drift = (rng_.chance(0.5) ? 1.0 : -1.0) *
                rng_.uniform(b.stockVolMin * 0.5, b.stockVolMax * 0.8);
    }
    if (rng_.chance(b.stockCrashProb)) drift -= b.stockCrashDrop; // 黑天鹅
    if (rng_.chance(b.stockBoomProb)) drift += b.stockBoomRise;   // 天降横财
    step(ctx, drift);
}

void StockSystem::applyExternalShock(GameContext& ctx, double mult) {
    if (mult <= 0.0) mult = 0.1;
    step(ctx, mult - 1.0);
}

void StockSystem::step(GameContext& ctx, double drift) {
    notice_.clear();
    price_ = std::clamp(price_ * (1.0 + drift), 0.5, 999.0);
    price_ = std::round(price_ * 100.0) / 100.0;
    history_.push_back(price_);
    if (history_.size() > 120) history_.erase(history_.begin());
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
    Json hist = Json::array();
    for (double p : history_) hist.push_back(p);
    return Json{
        {"price", price_},
        {"history", hist},
        {"shares", shares_},
        {"costBasis", costBasis_},
        {"principal", principal_},
        {"loan", loan_},
        {"leverageIndex", leverageIndex_},
    };
}

void StockSystem::fromJson(const Json& j) {
    price_ = ju::getOr(j, "price", price_);
    history_.clear();
    if (j.contains("history") && j["history"].is_array())
        for (const auto& p : j["history"]) history_.push_back(p.get<double>());
    if (history_.empty()) history_.push_back(price_);
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
}
