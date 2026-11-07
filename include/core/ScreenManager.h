#pragma once
// ============================================================
// [core/ScreenManager] 屏幕栈（组长 A 维护）
// 分工 7.2：标题/校园/市场/事件/周报/结局/存读档 以栈管理 + 弹窗层。
// State 模式的落点之一：栈顶屏幕决定"当前看到什么、接受什么输入"。
// 注意：pop/clear 采用"延迟销毁"（graveyard），允许屏幕在自己的
// handleInput 里安全地弹出自己（对象在本次输入处理结束前仍存活）。
// ============================================================
#include <memory>
#include <vector>

#include "core/IScreen.h"

class ScreenManager {
public:
    void push(std::unique_ptr<IScreen> screen); // 调用 onEnter
    void pop();                                 // 弹出栈顶（延迟销毁）
    void popAll();                              // 清空（延迟销毁）
    void flushGraveyard();                      // 真正销毁（每帧入口调用）

    IScreen* top() const { return stack_.empty() ? nullptr : stack_.back().get(); }
    bool empty() const { return stack_.empty(); }
    bool topIsModal() const { return !stack_.empty() && stack_.back()->isModal(); }

private:
    std::vector<std::unique_ptr<IScreen>> stack_;
    std::vector<std::unique_ptr<IScreen>> graveyard_;
};
