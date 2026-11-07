// ============================================================
// [main] 控制台垂直切片入口（组长 A）
// 用法：huangpu_sim.exe [--data 目录] [--seed N]
// 垂直切片流程：标题 → 新游戏 → 移动/买卖 → 行动点耗尽 → 周结算
//             → 事件弹窗 → 存档/读档 → 12 周后 → 结局 → 回标题
// ============================================================
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "core/Game.h"
#include "util/Console.h"

int main(int argc, char** argv) {
    console::init();

    std::string dataDir = "data";
    std::uint64_t seed = 20260901;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--data" && i + 1 < argc) dataDir = argv[++i];
        else if (a == "--seed" && i + 1 < argc) seed = std::strtoull(argv[++i], nullptr, 10);
    }

    // 兼容从 build/ 目录启动：向上找 data/
    namespace fs = std::filesystem;
    std::string saveDir = "saves";
    if (!fs::exists(dataDir + "/balance.json") && fs::exists("../data/balance.json")) {
        dataDir = "../data";
        saveDir = "../saves";
    }

    try {
        Game game(dataDir, saveDir, seed);
        game.run();
    } catch (const std::exception& e) {
        // 配置/数据错误：fail fast，给出可读信息
        std::cout << "[启动失败] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
