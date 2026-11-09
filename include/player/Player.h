#pragma once
// ============================================================
// [player/Player] 玩家（组员 C 主责；A 集成最小版）
// 组合关系：Player 拥有 Wallet 和 Inventory（分工 9 组合示例）。
// 订阅 TradeCompletedEvent 维护交易流水（Observer 的真实用例）。
// ============================================================
#include <string>
#include <vector>

#include "core/DomainEvents.h"
#include "core/ISaveable.h"
#include "player/Energy.h"
#include "player/Inventory.h"
#include "player/StudySystem.h"
#include "player/Wallet.h"

class ConfigManager;

class Player : public ISaveable {
public:
    explicit Player(const ConfigManager& cfg);

    // ---- 组合持有的子系统 ----
    const Wallet& wallet() const { return wallet_; }
    Wallet& wallet() { return wallet_; }
    const Inventory& inventory() const { return inventory_; }
    Inventory& inventory() { return inventory_; }

    // ---- 学业 ----
    int studyScore() const { return study_.score(); }
    void addStudy(int delta) { study_.setScore(study_.score() + delta); }  // 兼容 D 的 StudyEffect
    StudySystem& study() { return study_; }
    const StudySystem& study() const { return study_; }

    // ---- 体力 ----
    const EnergySystem& energy() const { return energy_; }
    EnergySystem& energy() { return energy_; }

    // ---- 股力（炒股模拟器的成长数值） ----
    int stockSkill() const { return stockSkill_; }
    void addStockSkill(int delta) {
        stockSkill_ += delta;
        if (stockSkill_ < 0) stockSkill_ = 0;
        if (stockSkill_ > stockSkillMax_) stockSkill_ = stockSkillMax_;
    }

    // ---- 交易流水（Observer 订阅入口） ----
    void onTradeCompleted(const TradeCompletedEvent& e);
    const std::vector<std::string>& ledger() const { return ledger_; }

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    Wallet wallet_;             // 封装：外部只能走 add/spend（债务只经 charge）
    Inventory inventory_;       // 组合：背包是玩家的一部分
    EnergySystem energy_;       // 组合：体力（炒股消耗，休息恢复）
    StudySystem study_;         // 组合：学业（平时分/期中期末/挂科）
    int stockSkill_;            // 股力：决定 K 线倾向的成长数值
    int stockSkillMax_;
    std::vector<std::string> ledger_; // 最近流水（存档保留，上限截断）
};
