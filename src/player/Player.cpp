#include "player/Player.h"

#include <algorithm>
#include <cstdio>

#include "core/ConfigManager.h"
#include "util/JsonUtil.h"

Player::Player(const ConfigManager& cfg)
    : inventory_(cfg.balance().maxSlots),
      energy_(cfg),
      study_(cfg),
      stockSkill_(cfg.balance().stockSkillStart),
      stockSkillMax_(cfg.balance().stockSkillMax) {}

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
        {"studyScore", study_.score()},   // 旧版本仍能读
        {"study", study_.toJson()},       // 新字段：failed
        {"stamina", energy_.toJson()},
        {"stockSkill", stockSkill_},
        {"ledger", ledger_},
    };
}

void Player::fromJson(const Json& j) {
    if (j.contains("wallet")) wallet_.fromJson(j["wallet"]);
    if (j.contains("inventory")) inventory_.fromJson(j["inventory"]);
    if (j.contains("study")) study_.fromJson(j["study"]);
    else study_.setScore(ju::getOr(j, "studyScore", study_.score()));  // 旧档
    stockSkill_ = ju::getOr(j, "stockSkill", stockSkill_);
    if (stockSkill_ < 0) stockSkill_ = 0;
    if (stockSkill_ > stockSkillMax_) stockSkill_ = stockSkillMax_;
    if (j.contains("stamina")) energy_.fromJson(j["stamina"]);
    ledger_.clear();
    if (j.contains("ledger") && j["ledger"].is_array())
        for (const auto& e : j["ledger"]) ledger_.push_back(e.get<std::string>());
    if (ledger_.size() > 200) ledger_.erase(ledger_.begin(), ledger_.begin() + static_cast<long>(ledger_.size() - 200));
}
