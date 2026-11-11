#pragma once
// ============================================================
// [core/Game] 游戏主控（组长 A 维护）
// 分工 7.2：组合根 + 主循环 + 流程编排。
// 主循环：resolveFlow → 栈顶屏幕 render → 读输入 → handleInput。
// 屏幕栈负责"看什么"，Game 负责"存档/结算/事件入队"等全局流程。
// ============================================================
#include <cstdint>
#include <deque>
#include <memory>
#include <string>

#include "core/ConfigManager.h"
#include "core/EventBus.h"
#include "core/Random.h"
#include "core/ScreenManager.h"
#include "core/WeekSettlement.h"
#include "save/SaveManager.h"
#include "util/Json.h"

class GameContext;
class GameEvent;
class ICommand;
class IScreen;

class Game {
public:
    Game(const std::string& dataDir, const std::string& saveDir, std::uint64_t seed);
    ~Game(); // 在 .cpp 定义（unique_ptr<GameContext> 需要完整类型）

    void run(); // 主循环（Ctrl+Z/EOF 也能干净退出）

    // ---- 屏幕层访问器 ----
    GameContext* ctx() { return ctx_.get(); }
    ConfigManager& config() { return config_; }
    ScreenManager& screens() { return screens_; }
    SaveManager& saves() { return saves_; }
    Random& rng() { return rng_; }
    EventBus& bus() { return bus_; }

    const WeekSettlement::Report& lastReport() const { return lastReport_; }
    const EndingDef* chosenEnding() const { return chosenEnding_; }

    // ---- 流程控制（由屏幕调用） ----
    void newGame();
    bool loadGame(int slot, std::string& err);
    bool saveGame(int slot, std::string& err);
    void performAction(std::unique_ptr<ICommand> cmd); // 执行一次行动（含事件摇取/昏迷/周翻转检测）
    void finishWeekNow();                              // [8] 快进到周日结算
    void checkBankruptcy();                            // 炒股爆仓欠债 → 立即进入破产结局
    void debugJumpToEnding();                          // 临时调试：直接跳到结局
    void requestEndWeek();                             // 周结算 + 周报/固定事件入队
    void backToTitle();
    void exitApp() { running_ = false; }

private:
    void resolveFlow();               // 每帧入口：保证栈顶合理、弹窗依序入栈
    void pushScreen(std::unique_ptr<IScreen> screen);
    void queueEventDialog(const GameEvent* ev);
    void queueWeeklyReport();
    std::string readLine();

    ConfigManager config_;
    Random rng_;
    EventBus bus_;
    SaveManager saves_;
    ScreenManager screens_;
    std::unique_ptr<GameContext> ctx_;
    std::deque<std::unique_ptr<IScreen>> pendingDialogs_; // 待弹窗队列（周报/事件）
    WeekSettlement::Report lastReport_;
    const EndingDef* chosenEnding_ = nullptr;
    bool running_ = true;
    bool endingPending_ = false;
};
