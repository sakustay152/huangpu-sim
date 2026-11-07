// ============================================================
// [main] headless 模拟器入口（组长 A）
// 用法：
//   simulate.exe --simulate 1000 --seed 42 --strategy all --out sim_result.csv
//   --strategy trader|worker|mixed|all（all = 三种各跑一遍，直接对比）
// 不打开任何窗口，与游戏共用同一套规则类（GameContext/Session/TradeSystem）。
// ============================================================
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core/ConfigManager.h"
#include "simulate/SimulationRunner.h"

int main(int argc, char** argv) {
    int count = 200;
    std::uint64_t seed = 42;
    std::string strategy = "all";
    std::string out = "sim_result.csv";
    std::string dataDir = "data";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--simulate" && i + 1 < argc) count = std::atoi(argv[++i]);
        else if (a == "--seed" && i + 1 < argc) seed = std::strtoull(argv[++i], nullptr, 10);
        else if (a == "--strategy" && i + 1 < argc) strategy = argv[++i];
        else if (a == "--out" && i + 1 < argc) out = argv[++i];
        else if (a == "--data" && i + 1 < argc) dataDir = argv[++i];
    }
    namespace fs = std::filesystem;
    if (!fs::exists(dataDir + "/balance.json") && fs::exists("../data/balance.json"))
        dataDir = "../data";

    try {
        ConfigManager cfg(dataDir);
        SimulationRunner runner(cfg);

        std::vector<std::string> strategies;
        if (strategy == "all") strategies = {"trader", "worker", "mixed"};
        else strategies = {strategy};

        std::vector<SimGameResult> all;
        for (const auto& s : strategies) {
            std::cout << ">> 运行策略 " << s << " x" << count << " 局（种子 " << seed << " 起）…\n";
            auto rs = runner.runGames(s, seed, count);
            all.insert(all.end(), rs.begin(), rs.end());
        }

        SimulationRunner::writeCsv(out, all);
        SimulationRunner::printSummary(all);
        std::cout << "\n明细已写入 " << out << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "[模拟失败] " << e.what() << "\n";
        return 1;
    }
}
