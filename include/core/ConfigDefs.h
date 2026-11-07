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
    int apPerWeek = 6;            // 每周行动点
    int maxSlots = 30;            // 背包格数

    int studyStart = 40;          // 平时分初始
    int studyGain = 6;            // 每次学习 +
    int studyDecay = 4;           // 一周没学习 -
    int passScore = 60;           // 及格线
    int warnScore = 50;           // 期中警告线
    int scholarshipScore = 85;    // 奖学金平时分线
    int scholarship = 1000;       // 奖学金金额
    int retakeFee = 2000;         // 补考费

    int socialGain = 15;          // 每次社交好感 +
    int restAffinity = 3;         // 休息卧谈好感 +

    double eventChance = 0.22;    // 每次行动后随机事件概率
    double rumorAccuracy = 0.8;   // 情报准确率
    double trendAmp = 0.22;       // 周趋势幅度
    double noiseAmp = 0.06;       // 日噪声幅度
    double seasonAmp = 0.35;      // 需求期涨幅

    std::map<std::string, std::pair<int, int>> jobs; // 地点→打工日薪区间
    double kaoyanDiscount = 0.9;  // 学长线：考研资料进货折扣
    double innerTicketBonus = 1.25; // 学姐线：演出门票卖出加成
    double seniorWorkBonus = 1.2; // 学长线：打工日薪加成
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

// ---- endings.json：结局定义 ----
struct EndingDef {
    std::string id, title, text;
    int priority = 99;            // 越小越优先判定
    std::vector<Json> conditions; // 条件 JSON（ConditionFactory 解释）
};
