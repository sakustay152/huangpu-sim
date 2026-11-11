#pragma once
// ============================================================
// [core/ConfigDefs] 配置数据结构（组长 A 维护）
// 数值与内容全部来自 data/*.json（数据驱动，分工 8.2），
// 本文件只定义"配置长什么样"，代码不硬编码任何数值/文案。
// ============================================================
#include <map>
#include <string>
#include <vector>

#include "util/Json.h"

// ---- balance.json：全局数值锚点（分工 3.4 表） ----
struct BalanceDef {
    int startMoney = 3000;        // 启动资金
    int weeklyExpense = 250;      // 每周支出
    int targetMoney = 12000;      // 目标资产
    int weeks = 18;               // 学期总周数（16 教学周 + 2 考试周）
    int teachingWeeks = 16;       // 教学周数
    int midtermWeek = 8;          // 期中预警周
    int examStartWeek = 17;       // 考试周起始（第 17-18 周）
    int examStudyBonus = 3;       // 考试周图书馆冲刺加成
    int maxSlots = 30;            // 背包格数

    // ---- v2 时间系统：24 小时制，行动按"小时 + 体力"计费 ----
    int moveStamina = 5;          // 移动耗体力
    int moveHours = 1;            // 移动耗小时
    int studyStamina = 10;        // 学习耗体力
    int studyHours = 2;           // 学习耗小时
    int socialStamina = 5;        // 社交耗体力
    int socialHours = 2;          // 社交耗小时
    int workHours = 8;            // 一次打工班次（8 小时）
    int tradeHours = 1;           // 商品买卖一趟市场耗小时
    int learnStockHours = 2;      // 学股市知识耗小时
    int sleepFullHours = 8;       // 睡满该小时数 → 体力强制回满
    double insomniaChance = 0.10; // 睡觉失眠概率（等效睡眠时间 -2 小时）
    int insomniaPenaltyHours = 2;
    int faintHospitalFee = 50;    // 昏迷送校医院的诊疗费
    int faintMaxStaminaLoss = 10; // 昏迷扣除的体力上限（周结算恢复）

    int studyStart = 40;          // 平时分初始
    int studyGain = 3;            // 每次学习 +（v2：2 小时一节自习）
    int studyDecay = 4;           // 一周没学习 -
    int passScore = 60;           // 及格线
    int warnScore = 50;           // 期中警告线
    int scholarshipScore = 85;    // 奖学金平时分线
    int scholarship = 1000;       // 奖学金金额
    int retakeFee = 2000;         // 补考费
    // （v2：社交好感改由选项池给出 +5/0/-5；休息由"睡觉"机制取代）

    double eventChance = 0.22;    // 每次行动后随机事件概率
    double rumorAccuracy = 0.8;   // 情报准确率
    double trendAmp = 0.22;       // 周趋势幅度
    double noiseAmp = 0.06;       // 日噪声幅度
    double seasonAmp = 0.35;      // 需求期涨幅

    std::map<std::string, std::pair<int, int>> jobs; // 地点→打工日薪区间
    // ---- 打工岗位与体力（C，W3） ----
    std::map<std::string, std::string> jobNames;   // 地点→岗位名（"default" 兜底）
    std::map<std::string, int>         jobStamina; // 地点→每次打工耗体力
    double lowEnergyPayRate = 0.5;                 // 低体力时工资系数
    // ---- 学业：熬夜（C，W4 用） ----
    int studyNightBonus = 3;                       // 熬夜比普通多 +3
    int studyNightStamina = 15;                    // 熬夜耗体力
    double kaoyanDiscount = 0.9;  // 学长线：考研资料进货折扣
    double innerTicketBonus = 1.25; // 学姐线：演出门票卖出加成
    double seniorWorkBonus = 1.2; // 学长线：打工日薪加成

    // ---- 炒股模拟器（灵感：bilibili 炒外汇模拟器；新手失败率调高） ----
    double stockStartPrice = 10.0;     // 标的「浦江文旅（SHU）」起始价
    double stockVolMin = 0.01;         // 单步最小波动
    double stockVolMax = 0.06;         // 单步最大波动
    double stockFailProb = 0.62;       // 持仓时下跌概率（庄家最爱大一新生）
    double stockCrashProb = 0.03;      // 黑天鹅暴跌概率
    double stockCrashDrop = 0.25;      // 暴跌幅度
    double stockBoomProb = 0.02;       // 暴涨概率
    double stockBoomRise = 0.30;       // 暴涨幅度
    double stockFeeRate = 0.005;       // 手续费率
    double stockLoanWeeklyRate = 0.05; // 融资周利率
    std::vector<int> stockLeverages = {1, 2, 5}; // 可选杠杆倍数
    // ---- 股力（学股市知识提升；决定 K 线倾向，但不保证赚钱/亏钱） ----
    int stockSkillStart = 5;           // 初始股力
    int stockSkillMax = 100;           // 股力上限
    double stockSkillFactor = 0.003;   // 每点股力降低的收阴概率
    double stockSkillFailFloor = 0.25; // 满股力时收阴概率下限（仍可能亏钱）
    int learnStockStaminaCost = 30;    // 学股市知识耗体力（大量）
    int learnStockGainMin = 6;         // 每次学到的股力下限
    int learnStockGainMax = 10;        // 每次学到的股力上限
    int stockBarsKeep = 60;            // 保留的 K 线条数
    int stockIntraSteps = 6;           // 每根 K 线内部的子步数
    // ---- 体力（所有行动共用；昏迷扣上限，周结算恢复） ----
    int staminaMax = 100;
    int staminaStockCost = 15;         // 每次买卖耗体力
    int staminaLowThreshold = 30;      // 低体力阈值
    double staminaLowFailBonus = 0.15; // 低体力时额外下跌概率
    // ---- 破产 ----
    int bankruptcyDebt = 3000;         // 欠债超过该值 → 《破产清算》结局
};

// ---- goods.json：商品定义 ----
struct GoodsDef {
    std::string id, name, category;
    int basePrice = 1;            // 基准价
    double spread = 0.08;         // 买卖价差
    int slots = 1;                // 每件占格
    int limitPerDay = 15;         // 每地点每次到访限购（1 行动点=1 天，分工 3.4"每日 10-20 件"）
    std::vector<std::string> soldAt;              // 有售地点
    std::map<std::string, double> locFactor;      // 地点价格系数（缺省 1.0）
    std::vector<int> peakWeeks;                   // 需求高峰周
    std::string desc;
};

// ---- locations.json：地点定义 ----
struct LocationDef {
    std::string id, name, desc;
    int fare = 2;                 // 移动到本地的路费
    std::string npcId;            // 驻地 NPC（可为空）
};

// ---- npcs.json：NPC 定义 ----
struct NpcDef {
    std::string id, name, home, intro;
};

// ---- social.json：社交选项池（v2 社交三选一） ----
struct SocialPoolDef {
    std::vector<std::string> positive; // 好感 +5 的选项池
    std::vector<std::string> neutral;  // 好感不变的选项池
    std::vector<std::string> negative; // 好感 -5 的选项池
};

// ---- endings.json：结局定义 ----
struct EndingDef {
    std::string id, title, text;
    int priority = 99;            // 越小越优先判定
    std::vector<Json> conditions; // 条件 JSON（ConditionFactory 解释）
};
