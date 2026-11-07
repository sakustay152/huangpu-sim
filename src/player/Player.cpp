#include "player/Player.h"

#include <algorithm>
#include <cstdio>

#include "core/ConfigManager.h"
#include "util/JsonUtil.h"

Player::Player(const ConfigManager& cfg)
    : inventory_(cfg.balance().maxSlots),
      studyScore_(cfg.balance().studyStart) {}

void Player::onTradeCompleted(const TradeCompletedEvent& e) {
    const GoodsDef* g = nullptr; // 文本由调用方补充商品名太绕，这里只记 id/qty/金额
    (void)g;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s %s x%d 金额 %d",
                  e.isBuy ? "买入" : "卖出", e.goodsId.c_str(), e.qty, e.total);
    ledger_.push_back(buf);
    if (ledger_.size() > 200) ledger_.erase(ledger_.begin()); // 流水上限，防膨胀
}

Json Player::toJson() const {
    return Json{
        {"wallet", wallet_.toJson()},
        {"inventory", inventory_.toJson()},
        {"studyScore", studyScore_},
        {"ledger", ledger_},
    };
}

void Player::fromJson(const Json& j) {
    if (j.contains("wallet")) wallet_.fromJson(j["wallet"]);
    if (j.contains("inventory")) inventory_.fromJson(j["inventory"]);
    studyScore_ = ju::getOr(j, "studyScore", studyScore_);
    ledger_.clear();
    if (j.contains("ledger") && j["ledger"].is_array())
        for (const auto& e : j["ledger"]) ledger_.push_back(e.get<std::string>());
    if (ledger_.size() > 200) ledger_.erase(ledger_.begin(), ledger_.begin() + static_cast<long>(ledger_.size() - 200));
}
