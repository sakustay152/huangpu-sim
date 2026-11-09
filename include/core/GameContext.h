#pragma once
// ============================================================
// [core/GameContext] 游戏上下文 —— 组合根（组长 A 维护）
// 分工 7.2：把 Player(C)、Market(B)、EventScheduler(D)、TimeSystem(C)
// 等系统组合在一起；UI 与各系统通过 GameContext 的引用读写，
// 不反向依赖 UI；只做组合与转发，不写业务。
// 依赖注入：config/rng/bus 以引用注入，不用全局单例。
// ============================================================
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "util/Json.h"

class Player;
class Market;
class TimeSystem;
class EventScheduler;
class StoryFlags;
class NpcSystem;
class TradeSystem;
class StockSystem;
class EventBus;
class Random;
class ConfigManager;

class GameContext {
public:
    GameContext(const ConfigManager& config, Random& rng, EventBus& bus);
    ~GameContext();
    GameContext(const GameContext&) = delete;
    GameContext& operator=(const GameContext&) = delete;

    // 开新局：初始化各系统并广播 WeekStarted
    void initNewGame();

    // 存档聚合：把所有 ISaveable 系统打包成一个 JSON（SaveManager 落盘）
    Json toJson() const;
    // 从存档 JSON 恢复所有系统；失败抛异常（坏档由 SaveManager/Game 处理）
    void loadFromJson(const Json& save);

    // ---- 依赖注入的三大件 ----
    const ConfigManager& config;
    Random& rng;
    EventBus& bus;

    // ---- 各系统（组合，生命周期与上下文一致） ----
    std::unique_ptr<Player> player;         // C：现金/背包/平时分
    std::unique_ptr<Market> market;         // B：行情
    std::unique_ptr<TimeSystem> time;       // C：周/行动点
    std::unique_ptr<EventScheduler> events; // D：事件调度
    std::unique_ptr<StoryFlags> flags;      // D：剧情 flag
    std::unique_ptr<NpcSystem> npcs;        // D：好感
    std::unique_ptr<TradeSystem> trade;     // C：交易
    std::unique_ptr<StockSystem> stock;     // 炒股（手机随时交易，耗体力）

    // ---- 运行期共享状态 ----
    std::string locationId = "dorm";        // 当前地点
    std::vector<std::string> rumors;        // 本周看到的情报文本
    std::string lastFeedback;               // 最近一次行动的反馈（UI 显示用）

    // ---- 破产判定（炒股爆仓欠债） ----
    bool isBankrupt() const;                // 欠债超过破产线
    void updateBankruptcyFlag();            // 欠债超线 → 设置 bankrupt flag

private:
    void wireSubscriptions(); // EventBus 订阅（Observer 模式的接线处）
    std::vector<std::uint64_t> subscriptionIds_; // 析构时逐个退订，避免悬垂捕获
};
