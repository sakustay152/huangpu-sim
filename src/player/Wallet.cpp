#include "player/Wallet.h"

#include "util/JsonUtil.h"

void Wallet::fromJson(const Json& j) {
    int m = ju::getOr(j, "money", money_);
    money_ = m < 0 ? 0 : m; // 坏档兜底：现金永远不为负
}
