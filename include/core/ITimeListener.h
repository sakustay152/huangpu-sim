#pragma once
// ============================================================
// [core/ITimeListener] 时间监听接口（组长 A 维护，W1 冻结接口）
// 时间系统每走一步，关心行情/事件的系统通过本接口收到通知。
// Market 实现它；日后 EnergySystem、NewsSystem 也可实现。
// ============================================================
class ITimeListener {
public:
    virtual ~ITimeListener() = default;

    // 一个行动点被消耗（= 一天过去），行情噪声刷新、冲击衰减
    virtual void onDayTick() = 0;

    // 新的一周开始，行情趋势/限购刷新
    virtual void onWeekStart() = 0;
};
