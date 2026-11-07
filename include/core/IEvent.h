#pragma once
// ============================================================
// [core/IEvent] 事件接口（组长 A 维护，W1 冻结接口；D 实现）
// 分工 7.5：IEvent { canTrigger(ctx); apply(ctx); }
// GameEvent 抽象基类实现本接口，RandomEvent/FixedEvent/NpcEvent 继承。
// ============================================================
#include <string>

class GameContext;

class IEvent {
public:
    virtual ~IEvent() = default;

    // 当前上下文是否允许触发（周数/地点/flag/好感/资产等条件）
    virtual bool canTrigger(const GameContext& ctx) const = 0;

    // 触发事件（默认取第一个选项；带选项的事件走 Session::applyChoice）
    virtual void apply(GameContext& ctx) const = 0;
};
