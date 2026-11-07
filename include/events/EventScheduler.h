#pragma once
// ============================================================
// [events/EventScheduler] 事件调度（D 主责；A 集成最小版）
// 分工 7.5：权重、冷却、条件（周数/地点/flag/好感/资产）、
// 每日触发（rollRandom）+ 每周固定节点（fixedForWeek）+ 延迟效果队列。
// ============================================================
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/ISaveable.h"
#include "util/Json.h"

class ConfigManager;
class GameContext;
class GameEvent;
class Random;

class EventScheduler : public ISaveable {
public:
    EventScheduler(const ConfigManager& cfg, Random& rng);

    // 从 events.json 构建全部事件（启动时调用一次；解析失败抛异常）
    void buildFromConfig();

    // 第 week 周开局的固定节点（已过滤"已触发/条件不符"）
    std::vector<const GameEvent*> fixedForWeek(int week, const GameContext& ctx) const;

    // 行动后摇一个随机事件；没有合格候选返回 nullptr
    const GameEvent* rollRandom(const GameContext& ctx);

    // 事件被选择后登记冷却（随机事件）
    void markTriggered(const GameEvent& ev);

    // 延迟效果队列（ScheduledEffect 调用）
    void scheduleEffect(int dueWeek, Json effectJson);
    void applyDueEffects(GameContext& ctx);

    // 周开始：冷却递减
    void onWeekStart();

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    const ConfigManager& cfg_;
    Random& rng_;
    std::vector<std::unique_ptr<GameEvent>> all_;
    std::map<std::string, int> cooldown_;          // 事件 id → 剩余冷却周
    std::vector<std::pair<int, Json>> scheduled_;  // 到期周 → 效果 JSON
};
