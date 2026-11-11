#pragma once
// ============================================================
// [ui/Screens] 垂直切片全部屏幕（A 的控制台实现；E 用 SFML 平行替换）
// Screen 继承树（分工 9 继承/多态示例）：
//   IScreen → ConsoleScreen → Title/Campus/Travel/Market/Inventory/
//                             EventDialog/WeeklyReport/SaveLoad/Ending/Help
// 每个屏幕 = 一次"状态"：render() 决定显示，handleInput() 决定转移。
// ============================================================
#include <functional>
#include <string>
#include <vector>

#include "ui/ConsoleUI.h"

class GameEvent;

class TitleScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "title"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;
};

class CampusScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "campus"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;
};

class TravelScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "travel"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;
};

class MarketScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "market"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;

private:
    std::string feedback_ = "买低卖高，限购每周刷新；不耗行动点，随便比价。";
};

class InventoryScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "inventory"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;
};

// 社交三选一：从驻地 NPC 的选项池抽出 +5/0/-5 三个话题（不显示分值）
class SocialScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "social"; }
    void onEnter() override;
protected:
    void draw() override;
    void handleInput(const std::string& in) override;

private:
    struct SocialOption {
        std::string text;
        int delta = 0;
    };
    std::vector<SocialOption> options_; // 每次进入时抽取
    std::string npcId_;
    std::string npcName_;
    std::string feedback_;
};

// 炒股屏：bilibili fx-simple 风格 —— K 线图 + 鼠标点击按钮交易/杠杆/学股市知识
// 真实控制台下接管输入循环（ENABLE_MOUSE_INPUT），行输入重定向时回退键盘命令
class StockScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "stock"; }
    void onEnter() override; // 接管输入：鼠标点击按钮；退出后自弹栈
protected:
    void draw() override;
    void handleInput(const std::string& in) override; // 键盘回退模式

private:
    struct Button {
        int row = -1;
        int c0 = 0;
        int c1 = 0;
        std::function<void()> action;
    };
    std::vector<Button> buttons_; // draw() 时登记，点击按行列命中
    std::string feedback_ = "股价随你的每次行动与周日结算走一根 K 线；买卖耗体力，新手慎用杠杆。";
    bool lineFallback_ = false;

    void clickBuy(int yuan);
    void clickSellAll();
    void clickCycleLeverage();
    void clickLearn();
};

class EventDialogScreen : public ConsoleScreen {
public:
    EventDialogScreen(Game& game, const GameEvent* ev) : ConsoleScreen(game), ev_(ev) { modal_ = true; }
    const char* id() const override { return "event"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;

private:
    const GameEvent* ev_;
    bool resolved_ = false;
    std::string resultText_;
};

class WeeklyReportScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "report"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;
};

class SaveLoadScreen : public ConsoleScreen {
public:
    SaveLoadScreen(Game& game, bool saveMode) : ConsoleScreen(game), saveMode_(saveMode) { modal_ = true; }
    const char* id() const override { return "saveload"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;

private:
    bool saveMode_;
    std::string feedback_;
};

class EndingScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "ending"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;

private:
    bool askedRestart_ = false; // 结局文本看过之后 → 弹出重新开局选择
    std::string feedbackAfterRestart_;
};

class HelpScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "help"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;
};
