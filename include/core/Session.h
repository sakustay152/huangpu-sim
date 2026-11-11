#pragma once
// ============================================================
// [core/Session] 行动管道（组长 A 维护）
// 交互版（console/SFML）与 headless 模拟器共用的"一次行动"逻辑：
//   校验行动点 → 执行命令 → 时间推进 → 行情日刷新 → 概率摇事件
// 事件如何被"选择"由调用方决定：交互版弹对话框让玩家选，
// 模拟器由 Bot 策略选 —— 同一套规则，两种前端。
// ============================================================
#include <string>

#include "core/ICommand.h"

class GameContext;
class GameEvent;

struct ActionOutcome {
    ActionResult result;
    const GameEvent* event = nullptr; // 本行动触发的随机事件（可能为空）
    bool fainted = false;             // 熬过凌晨 2 点 → 昏迷送校医院
    bool weekAdvanced = false;        // 本行动跨越了周日 → 需要周结算
};

class Session {
public:
    // 执行一次行动（可能触发随机事件；事件由调用方解决）
    static ActionOutcome runAction(GameContext& ctx, const ICommand& cmd);

    // 应用事件的某个选项，返回反馈文本；同时登记冷却/一次性标记
    static std::string applyChoice(GameContext& ctx, const GameEvent& ev, int choiceIdx);
};
