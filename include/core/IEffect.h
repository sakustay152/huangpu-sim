#pragma once
// ============================================================
// [core/IEffect] 事件效果接口（组长 A 维护，W1 冻结接口；D 实现）
// 分工 7.5：效果用数据描述（JSON），由 EffectFactory 创建具体效果类，
// 避免每个事件写一段代码。返回值是给玩家看的反馈文本。
// estimateMoneyDelta() 供 headless 模拟器的 Bot 估算选项收益。
// ============================================================
#include <string>

class GameContext;

class IEffect {
public:
    virtual ~IEffect() = default;

    // 施加效果，返回反馈文本（如 "现金 +120"）
    virtual std::string apply(GameContext& ctx) const = 0;

    // 对现金影响的粗略估计（模拟器 Bot 用；默认 0）
    virtual int estimateMoneyDelta() const { return 0; }
};
