#pragma once
// ============================================================
// [player/StudySystem] 学业（组员 C）
// 分工 7.4：平时分/学习效率/期中期末判定/挂科。
// 约定：onWeekEnd 只改自己的分数与挂科标记，钱由 WeekSettlement 落地。
// ============================================================
#include <string>

#include "core/ISaveable.h"
#include "util/Json.h"

class ConfigManager;
class EnergySystem;

enum class StudyMode { Normal, AllNighter };

struct StudyWeekOutcome {
    int decay = 0;              // 本周没学习扣的分
    bool midtermWarn = false;   // 第 8 周 < 50
    bool failedNow = false;     // 第 18 周 < 60
    int retakeFee = 0;          // 补考费
    int scholarship = 0;        // 平时分 >= 85
};

class StudySystem : public ISaveable {
public:
    explicit StudySystem(const ConfigManager& cfg);

    int score() const { return score_; }
    bool failed() const { return failed_; }
    void setScore(int v);   // 旧档迁移/测试用，业务代码别调

    // 返回实际获得的分；体力不够熬夜时自动退化为普通学习，并写进 msg
    int study(StudyMode mode, EnergySystem& energy, int week, std::string& msg);

    // 周结算：扣分 + 期中/期末判定（只改自身状态）
    StudyWeekOutcome onWeekEnd(int week, bool studiedThisWeek);

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    const ConfigManager& cfg_;
    int score_;
    bool failed_ = false;
};
