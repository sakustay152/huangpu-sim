#include "time/TimeSystem.h"

#include "core/ConfigManager.h"
#include "util/JsonUtil.h"

#include <cstdio>

TimeSystem::TimeSystem(const ConfigManager& cfg)
    : weeks_(cfg.balance().weeks) {}

const char* TimeSystem::dayName() const {
    static const char* names[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    int d = day_;
    if (d < 0) d = 0;
    if (d > 6) d = 6;
    return names[d];
}

std::string TimeSystem::clockText() const {
    char buf[32];
    if (hour_ < 24) {
        std::snprintf(buf, sizeof(buf), "%02d:00", hour_);
        return buf;
    }
    std::snprintf(buf, sizeof(buf), "次日 %02d:00", hour_ - 24);
    return buf;
}

bool TimeSystem::advanceDay() {
    bool weekRolled = false;
    lastWeekStudied_ = studiedThisWeek_;
    ++day_;
    if (day_ > 6) {
        day_ = 0;
        ++week_;
        studiedThisWeek_ = false;
        weekRolled = true;
    }
    hour_ = kWakeHour;
    return weekRolled;
}

Json TimeSystem::toJson() const {
    return Json{
        {"week", week_},
        {"day", day_},
        {"hour", hour_},
        {"studiedThisWeek", studiedThisWeek_},
        {"lastWeekStudied", lastWeekStudied_},
    };
}

void TimeSystem::fromJson(const Json& j) {
    week_ = ju::getOr(j, "week", week_);
    day_ = ju::getOr(j, "day", day_);
    hour_ = ju::getOr(j, "hour", hour_);
    studiedThisWeek_ = ju::getOr(j, "studiedThisWeek", studiedThisWeek_);
    lastWeekStudied_ = ju::getOr(j, "lastWeekStudied", lastWeekStudied_);
    if (hour_ < kWakeHour) hour_ = kWakeHour;
    if (hour_ > kCurfewHour) hour_ = kCurfewHour;
}
