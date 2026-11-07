#pragma once
// ============================================================
// [time/TimeSystem] 时间与行动点（组员 C 主责；A 集成最小版）
// 分工 3.2：12 周 × 每周 6 行动点（周一~周六），行动点耗尽自动周日结算。
// 一个行动点 = 一天。周日自动结算由调用方（Game/Sim）驱动。
// ============================================================
#include "core/ISaveable.h"
#include "util/Json.h"

class ConfigManager;

class TimeSystem : public ISaveable {
public:
    explicit TimeSystem(const ConfigManager& cfg);

    int week() const { return week_; }
    int actionsLeft() const { return actionsLeft_; }
    int apPerWeek() const { return apPerWeek_; }

    // 今天周几（0=周一 … 5=周六，本学期第 dayIndex+1 个行动日）
    int dayIndex() const { return apPerWeek_ - actionsLeft_; }
    const char* dayName() const;

    bool canAct() const { return actionsLeft_ > 0; }
    bool isSemesterOver() const { return week_ > weeks_; }

    // 消耗 1 行动点（调用前先 canAct()）
    void spendAction() { if (actionsLeft_ > 0) --actionsLeft_; }

    // 周日结算后进入新的一周
    void startNextWeek() { ++week_; actionsLeft_ = apPerWeek_; studiedThisWeek_ = false; }

    // 本周是否学习过（周结算扣平时分用）
    void markStudied() { studiedThisWeek_ = true; }
    bool studiedThisWeek() const { return studiedThisWeek_; }

    // 提前结束本周（模拟器/特殊事件兜底）
    void forceEndWeek() { actionsLeft_ = 0; }

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    int weeks_;
    int apPerWeek_;
    int week_ = 1;
    int actionsLeft_ = 6;
    bool studiedThisWeek_ = false;
};
