#pragma once
// ============================================================
// [core/ICommand] 玩家行动命令接口（组长 A 维护）
// 分工 7.2：Command 模式 —— 移动/打工/学习/社交/休息都是命令对象，
// 可记录（交易流水/行动日志）、可回放，不做撤销。
// 校验在命令内部完成，失败返回 ok=false 且不消耗行动点。
// ============================================================
#include <string>

class GameContext;

struct ActionResult {
    bool ok = false;
    bool consumedTime = false; // 是否消耗了时间（小时数，各命令按配置扣除）
    std::string msg;           // 给玩家看的反馈
};

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual std::string name() const = 0;

    virtual ActionResult execute(GameContext& ctx) const = 0;
};
