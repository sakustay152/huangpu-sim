# 《黄埔江上又一人》· 校园浮生录

> 面向对象程序设计课程项目 · 模拟经营方向（类《北京浮生记》）
> 上海书大校园舞台 · 12 周学期制 · 钱 / 学业 / 人际 三线经营

**一句话**：大一新生决定这学期不再向家里要钱，在 16 个教学周 + 2 周考试周（共 18 周）里跑地点、看行情、低买高卖、打工、学习、社交，期末攒够下学期的学费，并决定自己成为什么样的人。

---

## 一键构建与运行（Windows + MSVC）

```bat
:: 1. 构建（无需 CMake / vcpkg，MSVC 直编，third_party 已内置 nlohmann/json）
build.bat

:: 2. 运行（三个产物都在 build\ 下）
cd build
huangpu_sim.exe        REM 游戏本體（控制台垂直切片）
huangpu_simulate.exe   REM headless 模拟器（不开窗口直接跑批）
huangpu_tests.exe      REM 规则测试（33 项断言）
```

也可以用 CMake（组员用 IDE / 其它机器时）：

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
```

## 模拟器（报告"结果分析"的数据来源）

```bat
cd build
huangpu_simulate.exe --simulate 1000 --seed 42 --strategy all --out sim_result.csv
```

- `--strategy trader|worker|mixed|all`，`--seed` 固定可复现（同一套规则类，与游戏共用）。
- 已校准的一组结果（16+2 周学期、1000 局 × 3 策略，见 `docs/sim_result.csv`）：

| 策略 | 校园合伙人 | 站稳脚跟 | 账单之外 | 平均总资产 | 挂科率 |
| --- | --- | --- | --- | --- | --- |
| mixed（混合） | 0% | **34.2%** | 65.8% | ¥11,478 | 0% |
| trader（只倒卖） | 0% | 0% | 100% | **¥13,108**（最高） | 0% |
| worker（只打工） | 0% | 0% | 100% | ¥7,683 | 0% |

> 结论：只倒卖钱最多但没有朋友 → 依旧"账单之外"；只打工最稳但够不到目标；
> **混合策略才是好结局路径，且不存在单一最优解** —— 这正是三线经营的设计目标。

## 玩法（垂直切片已实现）

- **时间**：对齐真实上海大学 —— **16 个教学周 + 第 17、18 周考试周（共 18 周）**，每周 6 行动点
  （周一~周六，1 行动点 = 1 天），耗尽自动周日结算。
- **行动**：移动（路费）/ 打工 / 学习 / 社交 / 休息各耗 1 点；**买卖不耗点**；
  考试周图书馆学习有冲刺加成（+9 分）。
- **市场**：6 地点 × 12 商品，`mid = base × 地点系数 × 季节 × 周趋势 × 事件冲击 × 日噪声`，
  买卖价差 8%（大件 5%），每地点每日限购，背包 30 格；
  期中（第 8 周）与考试季（第 15~18 周）教材/笔记/咖啡/打印额度需求暴涨。
- **事件**：5 固定节点（开学/期中第 8 周/双十一第 10 周/考试周第 17、18 周）+ 10 随机事件 + 3 条 NPC 小线，
  全部 JSON 数据驱动。
- **情报**：室友每周一条，80% 准确（先于行情生成，直接影响下周趋势）。
- **结局**（endings.json 按 priority 判定，条件与事件共用一套 Condition 体系）：
  - 《站稳脚跟》：总资产 ≥ ¥12,000 + 不挂科 + 任一 NPC 好感 ≥ 60
  - 《校园合伙人》（隐藏）：总资产 ≥ ¥13,500 + 学姐 ≥ 80 + 社团 flag
  - 《账单之外》：以上都不满足（不是 Game Over，是带遗憾的开放结局）

## 目录结构

```text
HuangpuSim/
  CMakeLists.txt  build.bat  run_demo.bat  README.md
  include/ src/            ← 代码（include/core economy player events story time ui save simulate util）
  main/                    ← main_console.cpp（游戏）/ main_simulate.cpp（模拟器）
  tests/test_main.cpp      ← 33 项规则断言（钱守恒/容量/价格边界/存档往返/结局判定/模拟不变量）
  data/                    ← balance goods locations events npcs endings（全部数据驱动，改数值不重编译）
  third_party/nlohmann/    ← 内置 JSON 单头（离线可编译）
  docs/                    ← 组长交付说明 / 架构说明 / 存档格式 / 模拟数据
  build/                   ← 构建产物（demo_input.txt 为脚本化回归输入）
```

## 组内协作入口

- **分工与交接**：`docs/组长交付说明.md`（组长 A 已完成/占位的模块清单，B/C/D/E 从哪里接管）
- **架构与 OOP 对照**：`docs/架构说明.md`（报告第 9 节素材：六大设计模式的落点）
- **存档格式**：`docs/存档格式.md`
- **数值校准记录**：`docs/组长交付说明.md` 第 5 节（跑 1000 局 → 改 JSON → 再提交的完整过程）

## 编码约定

- 源码 UTF-8（MSVC `/utf-8`），C++17，无第三方依赖（除内置 nlohmann/json）
- 逻辑错误用返回值/错误码，不用异常做流程控制；配置错误启动时 fail-fast
- 所有随机走统一 `Random`（splitmix64）+ 种子，bug 可复现，随机状态随存档序列化

