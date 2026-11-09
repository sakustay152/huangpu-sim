#include "player/StudySystem.h"

#include <algorithm>

#include "core/ConfigManager.h"
#include "player/Energy.h"
#include "util/JsonUtil.h"

StudySystem::StudySystem(const ConfigManager& cfg)
    : cfg_(cfg), score_(cfg.balance().studyStart) {}

void StudySystem::setScore(int v) { score_ = std::clamp(v, 0, 100); }

int StudySystem::study(StudyMode mode, EnergySystem& energy, int week, std::string& msg) {
    const BalanceDef& b = cfg_.balance();
    bool night = (mode == StudyMode::AllNighter);

    // 熬夜要体力；不够就退化成普通自习（文案告诉玩家）
    const bool fellBack = night && !energy.spend(b.studyNightStamina);
    if (fellBack) night = false;

    // msg 每次都要重写：它是出参，调用方的 std::string 会复用
    msg = fellBack ? "太累了，熬夜看不进去，改成普通自习。"
                   : (night ? "在图书馆刷夜" : "泡了一天图书馆");

    int gain = b.studyGain + (night ? b.studyNightBonus : 0);
    if (week >= b.examStartWeek) gain += b.examStudyBonus;   // 考试周冲刺

    const int before = score_;
    score_ = std::min(100, score_ + gain);
    return score_ - before;
}

StudyWeekOutcome StudySystem::onWeekEnd(int week, bool studiedThisWeek) {
    StudyWeekOutcome o;
    const BalanceDef& b = cfg_.balance();

    if (week > 1 && !studiedThisWeek) {
        o.decay = std::min(b.studyDecay, score_);   // 不会扣成负数
        score_ -= o.decay;
    }
    if (week == b.midtermWeek && score_ < b.warnScore) o.midtermWarn = true;
    if (week == b.weeks) {
        if (score_ < b.passScore) {
            failed_ = true;
            o.failedNow = true;
            o.retakeFee = b.retakeFee;
        } else if (score_ >= b.scholarshipScore) {
            o.scholarship = b.scholarship;
        }
    }
    return o;
}

Json StudySystem::toJson() const {
    return Json{{"score", score_}, {"failed", failed_}};
}

void StudySystem::fromJson(const Json& j) {
    score_ = std::clamp(ju::getOr(j, "score", score_), 0, 100);
    failed_ = ju::getOr(j, "failed", failed_);
}
