#pragma once
// ============================================================
// [simulate/SimulationRunner] headless 模拟器（组长 A）
// 分工 5：game --simulate 1000 --seed 42 --strategy mixed，
// 不开窗口直接跑批，输出 CSV（结局分布、平均资产、策略对比）。
// 用途：① 报告"结果分析"的真实数据；② 找 dominant strategy；③ 平衡校准。
// ============================================================
#include <string>
#include <vector>

class ConfigManager;

struct SimGameResult {
    std::uint64_t seed = 0;
    std::string strategy;
    std::string endingId;
    int finalCash = 0;      // 期末现金
    int portfolioValue = 0; // 期末库存估值
    int studyScore = 0;
    int maxAffinity = 0;
    bool failed = false;    // 是否挂科
    int minMoney = 0;       // 全程最低现金（破产观察）
};

class SimulationRunner {
public:
    explicit SimulationRunner(const ConfigManager& cfg) : cfg_(cfg) {}

    // 从 seed0 开始连跑 count 局（第 i 局种子 = seed0 + i，可复现）
    std::vector<SimGameResult> runGames(const std::string& strategyName,
                                        std::uint64_t seed0, int count);

    static void writeCsv(const std::string& path, const std::vector<SimGameResult>& results);
    static void printSummary(const std::vector<SimGameResult>& results);

private:
    SimGameResult runOne(const std::string& strategyName, std::uint64_t seed);

    const ConfigManager& cfg_;
};
