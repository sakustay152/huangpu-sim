#pragma once
// ============================================================
// [core/IScreen] 屏幕/界面接口（组长 A 维护，W1 冻结接口）
// 分工 8.1：标题、校园、市场、事件、周报、结局、存读档、设置
// 全部实现本接口，由 ScreenManager 以栈管理。
// A 的垂直切片用控制台实现；E 的 SFML 屏幕实现同一接口即可替换。
// ============================================================
#include <string>

class IScreen {
public:
    virtual ~IScreen() = default;

    virtual const char* id() const = 0;

    // 进入屏幕时调用一次（可做初始化/动画入口）
    virtual void onEnter() {}

    // 绘制当前帧（控制台=整屏文本；SFML=绘制窗口）
    virtual void render() = 0;

    // 处理一次输入（控制台=一行命令；SFML=窗口事件）
    virtual void handleInput(const std::string& input) = 0;

    // 模态屏幕（事件弹窗/周报）显示期间，下层屏幕只作为背景
    virtual bool isModal() const { return false; }
};
