#pragma once
// ============================================================
// [ui/Screens] 垂直切片全部屏幕（A 的控制台实现；E 用 SFML 平行替换）
// Screen 继承树（分工 9 继承/多态示例）：
//   IScreen → ConsoleScreen → Title/Campus/Travel/Market/Inventory/
//                             EventDialog/WeeklyReport/SaveLoad/Ending/Help
// 每个屏幕 = 一次"状态"：render() 决定显示，handleInput() 决定转移。
// ============================================================
#include <string>

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
};

class HelpScreen : public ConsoleScreen {
public:
    using ConsoleScreen::ConsoleScreen;
    const char* id() const override { return "help"; }
protected:
    void draw() override;
    void handleInput(const std::string& in) override;
};
