#include "core/ScreenManager.h"

void ScreenManager::push(std::unique_ptr<IScreen> screen) {
    if (!screen) return;
    graveyard_.clear(); // 上一次输入处理已结束，旧屏幕可以销毁了
    IScreen* raw = screen.get();
    stack_.push_back(std::move(screen));
    raw->onEnter();
}

void ScreenManager::pop() {
    if (stack_.empty()) return;
    graveyard_.push_back(std::move(stack_.back())); // 延迟销毁：等下一次 push 再释放
    stack_.pop_back();
}

void ScreenManager::popAll() {
    while (!stack_.empty()) pop();
}

void ScreenManager::flushGraveyard() {
    graveyard_.clear();
}
