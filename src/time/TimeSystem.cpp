#include "time/TimeSystem.h"

#include "core/ConfigManager.h"
#include "util/JsonUtil.h"

TimeSystem::TimeSystem(const ConfigManager& cfg)
    : weeks_(cfg.balance().weeks),
      apPerWeek_(cfg.balance().apPerWeek),
      actionsLeft_(cfg.balance().apPerWeek) {}

const char* TimeSystem::dayName() const {
    static const char* names[] = {"周一", "周二", "周三", "周四", "周五", "周六"};
    int d = dayIndex();
    if (d < 0) d = 0;
    if (d > 5) d = 5;
    return names[d];
}

Json TimeSystem::toJson() const {
    return Json{
        {"week", week_},
        {"actionsLeft", actionsLeft_},
        {"studiedThisWeek", studiedThisWeek_},
    };
}

void TimeSystem::fromJson(const Json& j) {
    week_           = ju::getOr(j, "week", week_);
    actionsLeft_    = ju::getOr(j, "actionsLeft", actionsLeft_);
    studiedThisWeek_ = ju::getOr(j, "studiedThisWeek", studiedThisWeek_);
}
