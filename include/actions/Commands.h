#pragma once
// ============================================================
// [actions/Commands] 玩家行动命令（Command 模式，分工 9）
// v2 时间系统：每个命令自己校验并扣除"体力 + 小时数"，
// 失败不消耗任何资源。睡觉推进到次日 8:00（周日凌晨触发周结算）。
// ============================================================
#include <string>

#include "core/ICommand.h"
#include "player/StudySystem.h" // 需要 StudyMode

// 移动到某地点（耗体力 + 1 小时，不再收路费）
class MoveCommand : public ICommand {
public:
    explicit MoveCommand(std::string to) : to_(std::move(to)) {}
    std::string name() const override { return "移动"; }
    ActionResult execute(GameContext& ctx) const override;
private:
    std::string to_;
};

// 在当前地点打工（食堂/超市/西门有正式岗位，其余为打零工；一次 8 小时班）
class WorkCommand : public ICommand {
public:
    std::string name() const override { return "打工"; }
    ActionResult execute(GameContext& ctx) const override;
};

// 图书馆学习（+3 平时分；熬夜模式多加分但耗体力，体力不足自动退化）
class StudyCommand : public ICommand {
public:
    explicit StudyCommand(StudyMode mode = StudyMode::Normal) : mode_(mode) {}
    std::string name() const override {
        return mode_ == StudyMode::AllNighter ? "熬夜学习" : "学习";
    }
    ActionResult execute(GameContext& ctx) const override;
private:
    StudyMode mode_;
};

// 宿舍睡觉（推进到次日 8:00；睡得越久体力回得越多，睡满 8 小时回满；
// 10% 概率失眠；熬过凌晨 2 点不睡会昏迷——由 Session 统一处理）
class SleepCommand : public ICommand {
public:
    std::string name() const override { return "睡觉"; }
    ActionResult execute(GameContext& ctx) const override;
};

// 学股市知识（耗大量体力 + 2 小时，提升股力）
class LearnStockCommand : public ICommand {
public:
    std::string name() const override { return "学股市知识"; }
    ActionResult execute(GameContext& ctx) const override;
};

// 社交：玩家在 SocialScreen 三选一后执行（好感 +5/0/-5 由选项池决定）
class SocialPickCommand : public ICommand {
public:
    SocialPickCommand(std::string npcId, std::string text, int delta)
        : npcId_(std::move(npcId)), text_(std::move(text)), delta_(delta) {}
    std::string name() const override { return "社交"; }
    ActionResult execute(GameContext& ctx) const override;
private:
    std::string npcId_;
    std::string text_;
    int delta_;
};
