#pragma once
// ============================================================
// [simulate/BotStrategy] headless 策略（组长 A；与 B/C 商定的接口）
// 分工 7.2：SimulationRunner 用策略接口跑批，与游戏共用同一套规则
// （同一 GameContext/Session/TradeSystem），保证"模拟即游戏"。
// IChoiceResolver：Bot 如何选事件选项（用 IEffect::estimateMoneyDelta 估收益）。
// ============================================================
#include <memory>
#include <string>

class GameContext;
class GameEvent;

class IChoiceResolver {
public:
    virtual ~IChoiceResolver() = default;
    virtual int chooseChoice(const GameEvent& ev, const GameContext& ctx) const = 0;
};

class BotStrategy {
public:
    virtual ~BotStrategy() = default;
    virtual std::string name() const = 0;

    // 执行一步：要么免行动点交易（买卖），要么消耗 1 行动点。
    // 返回本步是否消耗了行动点（模拟器用它判断"卡死"）。
    virtual bool takeTurn(GameContext& ctx) = 0;
};

// 工厂：trader / worker / mixed；未知名字抛异常
std::unique_ptr<BotStrategy> makeStrategy(const std::string& name);

// 模拟器默认的事件选项解析器（估算现金收益最大的选项）
const IChoiceResolver& defaultResolver();
