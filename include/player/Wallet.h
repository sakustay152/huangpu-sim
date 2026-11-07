#pragma once
// ============================================================
// [player/Wallet] 钱包（组员 C 主责；A 集成最小版）
// 封装：现金只能通过 add/spend 修改，spend 失败返回 false，
// 保证"钱不为负"这条不变量在唯一入口处成立（分工 7.4 单测点）。
// ============================================================
#include "core/ISaveable.h"

class Wallet : public ISaveable {
public:
    Wallet() = default;
    explicit Wallet(int money) : money_(money) {}

    int get() const { return money_; }
    bool canAfford(int amount) const { return money_ >= amount; }

    void add(int amount) { if (amount > 0) money_ += amount; }

    // 扣款：余额不足返回 false 且不扣（所有消费必须走这里）
    bool spend(int amount) {
        if (amount < 0 || money_ < amount) return false;
        money_ -= amount;
        return true;
    }

    Json toJson() const override { return Json{{"money", money_}}; }
    void fromJson(const Json& j) override;

private:
    int money_ = 0;
};
