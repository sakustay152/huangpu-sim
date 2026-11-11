#pragma once
// ============================================================
// [time/TimeSystem] 24 小时制时间系统（组长 A 重构 v2）
// - 每天 8:00 起床，行动消耗小时数（小时数由各命令按 balance 配置扣除）
// - hour = 26 表示次日凌晨 2 点：熬过这个点会昏迷（调用方在行动后检查）
// - advanceDay() 推进到次日 8:00；跨周日（周日→周一）返回 true，
//   由 Session/Game 触发周结算
// - studiedThisWeek / lastWeekStudied 供周结算的学业判定
// ============================================================
#include "core/ISaveable.h"
#include "util/Json.h"
#include <string>

class ConfigManager;

class TimeSystem : public ISaveable {
public:
    explicit TimeSystem(const ConfigManager& cfg);

    static constexpr int kCurfewHour = 26; // 次日凌晨 2 点（8 点记为 0，+18 小时）
    static constexpr int kWakeHour = 8;    // 每天起床时间

    int week() const { return week_; }            // 1..weeks（周日结算期间短暂 = weeks+1）
    int settledWeek() const { return week_ - 1; } // 正在结算哪一周（周翻转后调用）
    int day() const { return day_; }              // 0=周一 … 6=周日
    int hour() const { return hour_; }            // 8..26
    const char* dayName() const;
    std::string clockText() const;                // "14:00" / "次日 01:00"

    bool canAct() const { return hour_ < kCurfewHour && !isSemesterOver(); }
    bool isSemesterOver() const { return week_ > weeks_; }

    // 消耗小时数（可能越过 26 → 调用方随后检查昏迷）
    void spendHours(int h) { if (h > 0) hour_ += h; }

    // 推进到次日 8:00；跨周日（周日→周一）返回 true → 触发周结算
    bool advanceDay();

    // 本周是否学习过（周结算学业判定用）
    void markStudied() { studiedThisWeek_ = true; }
    bool studiedThisWeek() const { return studiedThisWeek_; }
    bool lastWeekStudied() const { return lastWeekStudied_; }

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    int weeks_;
    int week_ = 1;
    int day_ = 0;      // 0=周一 … 6=周日
    int hour_ = kWakeHour;
    bool studiedThisWeek_ = false;
    bool lastWeekStudied_ = false;
};
