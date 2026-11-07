#pragma once
// ============================================================
// [ui/ConsoleUI] 控制台屏幕基类（A 的垂直切片实现；E 将用 SFML 重写）
// 所有控制台屏幕继承 ConsoleScreen（→ IScreen），
// render() 固定为"清屏 + draw()"，子类只管 draw()/handleInput()。
// ============================================================
#include "core/IScreen.h"

class Game;

class ConsoleScreen : public IScreen {
public:
    explicit ConsoleScreen(Game& game) : game_(game) {}
    bool isModal() const override { return modal_; }

protected:
    void render() override;          // 清屏后调用 draw()
    virtual void draw() = 0;

    void header(const std::string& title) const; // ══ 标题 ══
    static void rule();                          // 分隔线
    static void line(const std::string& s = ""); // 一行文本

    Game& game_;
    bool modal_ = false;
};
