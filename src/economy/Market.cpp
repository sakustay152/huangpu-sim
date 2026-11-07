#include "economy/Market.h"

#include <algorithm>
#include <cmath>

#include "core/ConfigManager.h"
#include "core/Random.h"
#include "economy/PriceModels.h"
#include "util/JsonUtil.h"

Market::Market(const ConfigManager& cfg, Random& rng)
    : cfg_(cfg), rng_(rng), chain_(buildDefaultPriceChain(cfg.balance())) {}

Market::~Market() = default;

double Market::trendOf(const std::string& locId, const std::string& goodsId) const {
    auto it = trend_.find(locId);
    if (it == trend_.end()) return 0.0;
    auto it2 = it->second.find(goodsId);
    return it2 == it->second.end() ? 0.0 : it2->second;
}

double Market::noiseOf(const std::string& goodsId) const {
    auto it = noiseMult_.find(goodsId);
    return it == noiseMult_.end() ? 1.0 : it->second;
}

double Market::shockOf(const std::string& goodsId) const {
    auto it = shock_.find(goodsId);
    return it == shock_.end() ? 1.0 : it->second.first;
}

double Market::midPrice(const std::string& locId, const std::string& goodsId) const {
    const GoodsDef* g = cfg_.goods(goodsId);
    const LocationDef* l = cfg_.location(locId);
    if (!g || !l) return 1.0;
    PriceContext c{*g, *l, week_, trendOf(locId, goodsId), noiseOf(goodsId), shockOf(goodsId)};
    return chain_->price(c);
}

int Market::buyPrice(const std::string& locId, const std::string& goodsId) const {
    const GoodsDef* g = cfg_.goods(goodsId);
    if (!g) return 1;
    double mid = midPrice(locId, goodsId);
    return std::max(1, static_cast<int>(std::round(mid * (1.0 + g->spread))));
}

int Market::sellPrice(const std::string& locId, const std::string& goodsId) const {
    const GoodsDef* g = cfg_.goods(goodsId);
    if (!g) return 1;
    double mid = midPrice(locId, goodsId);
    return std::max(1, static_cast<int>(std::round(mid * (1.0 - g->spread))));
}

std::vector<Market::Listing> Market::listAt(const std::string& locId) const {
    std::vector<Listing> out;
    out.reserve(cfg_.goods().size());
    for (const auto& g : cfg_.goods()) {
        Listing li;
        li.goods = &g;
        li.buyPrice = buyPrice(locId, g.id);
        li.sellPrice = sellPrice(locId, g.id);
        li.canBuy = std::find(g.soldAt.begin(), g.soldAt.end(), locId) != g.soldAt.end();
        li.quotaLeft = quotaLeft(locId, g.id);
        out.push_back(li);
    }
    return out;
}

int Market::quotaLeft(const std::string& locId, const std::string& goodsId) const {
    auto it = quota_.find(locId);
    if (it == quota_.end()) {
        const GoodsDef* g = cfg_.goods(goodsId);
        return g ? g->limitPerDay : 0;
    }
    auto it2 = it->second.find(goodsId);
    if (it2 != it->second.end()) return it2->second;
    const GoodsDef* g = cfg_.goods(goodsId);
    return g ? g->limitPerDay : 0;
}

bool Market::consumeQuota(const std::string& locId, const std::string& goodsId, int qty) {
    if (quotaLeft(locId, goodsId) < qty) return false;
    quota_[locId][goodsId] -= qty;
    return true;
}

void Market::applyShock(const std::string& goodsId, double mult, int days) {
    if (!cfg_.goods(goodsId)) return;
    if (mult <= 0.0) mult = 1.0;
    if (days <= 0) days = 1;
    shock_[goodsId] = {mult, days};
}

void Market::onDayTick() {
    double amp = cfg_.balance().noiseAmp;
    for (const auto& g : cfg_.goods())
        noiseMult_[g.id] = 1.0 + rng_.uniform(-amp, amp);
    // 冲击衰减：按"行动点/天"倒数
    for (auto it = shock_.begin(); it != shock_.end();) {
        if (--it->second.second <= 0) it = shock_.erase(it);
        else ++it;
    }
    resetQuotas(); // 限购按"次到访/天"刷新（分工 3.4：每地点每日 10-20 件）
}

void Market::resetQuotas() {
    quota_.clear();
    for (const auto& g : cfg_.goods())
        for (const auto& loc : g.soldAt)
            quota_[loc][g.id] = g.limitPerDay;
}

void Market::onWeekStart() {
    double amp = cfg_.balance().trendAmp;
    // 全部 (loc, goods) 摇新趋势；情报命中的商品按情报方向偏移
    for (const auto& l : cfg_.locations()) {
        for (const auto& g : cfg_.goods()) {
            double t = rng_.uniform(-amp, amp);
            if (hasRumor_ && pendingRumor_.goodsId == g.id) {
                bool hit = rng_.chance(cfg_.balance().rumorAccuracy);
                double strength = rng_.uniform(0.4, 1.0) * amp;
                t = hit ? (pendingRumor_.up ? strength : -strength)
                        : rng_.uniform(-amp, amp);
            }
            trend_[l.id][g.id] = t;
        }
    }
    hasRumor_ = false;
    onDayTick(); // 新一周的第一天：噪声 + 限购
}

void Market::rollRumor() {
    const auto& goods = cfg_.goods();
    const GoodsDef* g = &goods[rng_.next() % goods.size()];
    bool up = rng_.chance(0.5);
    pendingRumor_ = {g->id, up};
    hasRumor_ = true;
    rumorTexts_.clear();
    rumorTexts_.push_back(std::string("【室友情报】听说下周「") + g->name + "」的价格可能要" +
                          (up ? "涨" : "跌") + "，准确率约 " +
                          std::to_string(static_cast<int>(cfg_.balance().rumorAccuracy * 100)) + "%");
}

const char* Market::trendArrow(const std::string& locId, const std::string& goodsId) const {
    double t = trendOf(locId, goodsId);
    if (t > 0.06) return "▲";
    if (t < -0.06) return "▼";
    return "─";
}

Json Market::toJson() const {
    Json trends = Json::object();
    for (const auto& [loc, gm] : trend_) {
        Json o = Json::object();
        for (const auto& [g, t] : gm) o[g] = t;
        trends[loc] = o;
    }
    Json noise = Json::object();
    for (const auto& [g, m] : noiseMult_) noise[g] = m;
    Json shocks = Json::array();
    for (const auto& [g, sd] : shock_)
        shocks.push_back(Json{{"goods", g}, {"mult", sd.first}, {"days", sd.second}});
    Json quotas = Json::object();
    for (const auto& [loc, gm] : quota_) {
        Json o = Json::object();
        for (const auto& [g, q] : gm) o[g] = q;
        quotas[loc] = o;
    }
    Json rumor;
    if (hasRumor_) rumor = Json{{"goods", pendingRumor_.goodsId}, {"up", pendingRumor_.up}};
    return Json{
        {"week", week_},
        {"trend", trends},
        {"noise", noise},
        {"shock", shocks},
        {"quota", quotas},
        {"pendingRumor", rumor},
        {"rumorTexts", rumorTexts_},
    };
}

void Market::fromJson(const Json& j) {
    week_ = ju::getOr(j, "week", week_);
    trend_.clear();
    if (j.contains("trend") && j["trend"].is_object())
        for (auto it = j["trend"].begin(); it != j["trend"].end(); ++it)
            for (auto it2 = it.value().begin(); it2 != it.value().end(); ++it2)
                trend_[it.key()][it2.key()] = it2.value().get<double>();
    noiseMult_.clear();
    if (j.contains("noise") && j["noise"].is_object())
        for (auto it = j["noise"].begin(); it != j["noise"].end(); ++it)
            noiseMult_[it.key()] = it.value().get<double>();
    shock_.clear();
    if (j.contains("shock") && j["shock"].is_array())
        for (const auto& e : j["shock"]) {
            std::string g = ju::getOr<std::string>(e, "goods", "");
            if (!cfg_.goods(g)) continue;
            shock_[g] = {ju::getOr(e, "mult", 1.0), ju::getOr(e, "days", 1)};
        }
    quota_.clear();
    if (j.contains("quota") && j["quota"].is_object())
        for (auto it = j["quota"].begin(); it != j["quota"].end(); ++it)
            for (auto it2 = it.value().begin(); it2 != it.value().end(); ++it2)
                quota_[it.key()][it2.key()] = it2.value().get<int>();
    hasRumor_ = false;
    if (j.contains("pendingRumor") && j["pendingRumor"].is_object()) {
        std::string g = ju::getOr<std::string>(j["pendingRumor"], "goods", "");
        if (cfg_.goods(g)) {
            pendingRumor_ = {g, ju::getOr(j["pendingRumor"], "up", true)};
            hasRumor_ = true;
        }
    }
    rumorTexts_.clear();
    if (j.contains("rumorTexts") && j["rumorTexts"].is_array())
        for (const auto& t : j["rumorTexts"]) rumorTexts_.push_back(t.get<std::string>());
}
