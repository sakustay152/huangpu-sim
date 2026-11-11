#pragma once
// ============================================================
// [player/GoalTracker] 目标进度快照（组员 C）
// 只读：给 UI 画进度条/周报用；不判结局（判定归 events/EndingJudge）。
// 总资产口径必须与 EndingJudge 的 totalAssetsMin 一致：
//   现金 + TradeSystem::estimatePortfolioValue()
// ============================================================
class GameContext;

struct GoalSnapshot {
    int cash = 0;
    int inventoryValue = 0;
    int totalAssets = 0;
    int targetMoney = 0;
    int studyScore = 0;
    int passScore = 0;
    int maxAffinity = 0;
    int week = 0;
    int weeks = 0;
    int day = 0;               // 0=周一 … 6=周日
    int hour = 0;              // 8..26（24 小时制）
    int stamina = 0;
    int staminaMax = 0;
    int slotsUsed = 0;
    int slotsMax = 0;
    bool failed = false;
};

class GoalTracker {
public:
    GoalSnapshot snapshot(const GameContext& ctx) const;
};
