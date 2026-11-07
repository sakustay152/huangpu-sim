#pragma once
// ============================================================
// [events/Condition] 触发条件（D 主责的机制；A 集成最小版）
// 条件用 JSON 描述（分工 7.5），ConditionFactory 创建；
// 同一套条件既用于事件触发，也用于结局判定 —— 一套机制两处复用。
// ============================================================
#include <memory>
#include <string>
#include <vector>

#include "core/ICondition.h"
#include "util/Json.h"

class GameContext;

// 周数区间 [min, max]
class WeekBetween : public ICondition {
public:
    WeekBetween(int min, int max) : min_(min), max_(max) {}
    bool test(const GameContext& ctx) const override;
private:
    int min_, max_;
};

// 当前地点
class LocationIs : public ICondition {
public:
    explicit LocationIs(std::string locId) : locId_(std::move(locId)) {}
    bool test(const GameContext& ctx) const override;
private:
    std::string locId_;
};

// 现金下限（事件用：手头现金）
class MinMoney : public ICondition {
public:
    explicit MinMoney(int v) : v_(v) {}
    bool test(const GameContext& ctx) const override;
private:
    int v_;
};

// 期末总资产下限（结局用：现金 + 库存按卖价估值，分工 3.5）
class TotalAssetsMin : public ICondition {
public:
    explicit TotalAssetsMin(int v) : v_(v) {}
    bool test(const GameContext& ctx) const override;
private:
    int v_;
};

// 拥有 / 未拥有 flag
class HasFlag : public ICondition {
public:
    explicit HasFlag(std::string flag) : flag_(std::move(flag)) {}
    bool test(const GameContext& ctx) const override;
private:
    std::string flag_;
};

// 指定 NPC 好感下限
class AffinityMin : public ICondition {
public:
    AffinityMin(std::string npcId, int v) : npcId_(std::move(npcId)), v_(v) {}
    bool test(const GameContext& ctx) const override;
private:
    std::string npcId_;
    int v_;
};

// 任一 NPC 好感下限
class AffinityAnyMin : public ICondition {
public:
    explicit AffinityAnyMin(int v) : v_(v) {}
    bool test(const GameContext& ctx) const override;
private:
    int v_;
};

// 背包非空
class InventoryNotEmpty : public ICondition {
public:
    bool test(const GameContext& ctx) const override;
};

// 恒真（默认结局用）
class AlwaysTrue : public ICondition {
public:
    bool test(const GameContext&) const override { return true; }
};

class ConditionFactory {
public:
    // {"type":"weekBetween","min":1,"max":12} 等；未知类型抛异常（fail fast）
    static std::unique_ptr<ICondition> create(const Json& j);
};
