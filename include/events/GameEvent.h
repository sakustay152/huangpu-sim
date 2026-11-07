#pragma once
// ============================================================
// [events/GameEvent] 事件继承体系（D 主责的机制；A 集成最小版）
// 继承树（分工 9）：
//   IEvent（冻结接口）
//     └─ GameEvent（抽象基类：条件 + 选项 + 效果）
//          ├─ RandomEvent（带权重/冷却，进随机池）
//          ├─ FixedEvent（固定周触发：开学/期中/双十一/期末）
//          └─ NpcEvent（好感门槛触发的 NPC 小线）
// 多态落点：canTrigger/apply/weight 虚函数。
// ============================================================
#include <memory>
#include <string>
#include <vector>

#include "core/IEvent.h"

class GameContext;
class ICondition;
class IEffect;

class GameEvent : public IEvent {
public:
    struct Choice {
        std::string text;
        std::vector<std::unique_ptr<IEffect>> effects;
    };

    std::string id, name, text;
    std::vector<Choice> choices;
    std::vector<std::unique_ptr<ICondition>> conditions;
    bool once = false; // 一次性事件（触发过就不再触发）

    bool canTrigger(const GameContext& ctx) const override;
    void apply(GameContext& ctx) const override; // 默认取第一个选项（headless 兜底）

    virtual int weight(const GameContext& ctx) const { (void)ctx; return 1; }
};

class RandomEvent : public GameEvent {
public:
    int weightValue = 10;      // 触发权重
    int cooldownWeeks = 2;     // 冷却（周）
    int weight(const GameContext& ctx) const override { (void)ctx; return weightValue; }
};

class FixedEvent : public GameEvent {
public:
    int week = 1; // 触发周
    bool canTrigger(const GameContext& ctx) const override;
};

class NpcEvent : public GameEvent {
public:
    std::string npcId;
    int minAffinity = 0;
    bool canTrigger(const GameContext& ctx) const override;
};
