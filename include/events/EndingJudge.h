#pragma once
// ============================================================
// [events/EndingJudge] 结局判定（D 主责；A 集成最小版）
// endings.json 按 priority 从小到大，第一个条件全通过的结局生效；
// 复用事件的条件工厂 —— 一套机制两处复用。
// ============================================================
#include "core/ConfigDefs.h"

class GameContext;

class EndingJudge {
public:
    // 永远返回非空（endings.json 里必须有 priority=99 的默认结局兜底）
    static const EndingDef* judge(const GameContext& ctx);
};
