#pragma once
// ============================================================
// [core/ICondition] 条件接口（组长 A 维护，W1 冻结接口；D 实现）
// 同一套条件既用于"事件能否触发"，也用于"结局判定"（EndingJudge），
// 数据全部来自 JSON —— 一套机制两处复用。
// ============================================================
#include <string>

class GameContext;

class ICondition {
public:
    virtual ~ICondition() = default;

    virtual bool test(const GameContext& ctx) const = 0;
};
