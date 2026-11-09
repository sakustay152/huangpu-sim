#pragma once
// ============================================================
// [actions/Commands] 玩家行动命令（组员 C 主责；A 集成最小版）
// Command 模式（分工 9）：每个行动一个类，校验在 execute 内部完成，
// 失败不消耗行动点。可记录/回放（不做撤销）。
// ============================================================
#include <string>

#include "core/ICommand.h"
#include "player/StudySystem.h"   // 需要 StudyMode

// 移动到某地点（耗 1 点 + 路费）
class MoveCommand : public ICommand {
public:
    explicit MoveCommand(std::string to) : to_(std::move(to)) {}
    std::string name() const override { return "移动"; }
    ActionResult execute(GameContext& ctx) const override;
private:
    std::string to_;
};

// 在当前地点打工（食堂/超市/西门有正式岗位，其余地点为打零工）
class WorkCommand : public ICommand {
public:
    std::string name() const override { return "打工"; }
    ActionResult execute(GameContext& ctx) const override;
};

// 图书馆学习（+平时分）；熬夜模式多加分但耗体力，体力不足自动退化
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

// 与驻地 NPC 社交（+好感）
class SocialCommand : public ICommand {
public:
    std::string name() const override { return "社交"; }
    ActionResult execute(GameContext& ctx) const override;
};

// 宿舍休息（和室友卧谈，小幅好感）
class RestCommand : public ICommand {
public:
    std::string name() const override { return "休息"; }
    ActionResult execute(GameContext& ctx) const override;
};

// 学股市知识（耗大量体力，提升股力——炒股 K 线倾向的成长数值）
class LearnStockCommand : public ICommand {
public:
    std::string name() const override { return "学股市知识"; }
    ActionResult execute(GameContext& ctx) const override;
};
