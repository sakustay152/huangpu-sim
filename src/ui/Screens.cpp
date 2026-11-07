#include "ui/Screens.h"

#include <iostream>

#include "actions/Commands.h"
#include "core/ConfigManager.h"
#include "core/Game.h"
#include "core/GameContext.h"
#include "core/Session.h"
#include "economy/Market.h"
#include "events/GameEvent.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"
#include "util/Console.h"
#include "util/StringUtils.h"

namespace {

std::string money(int v) { return "¥" + std::to_string(v); }

} // namespace

// ================= 标题 =================

void TitleScreen::draw() {
    header("《黄埔江上又一人》 · 校园浮生录");
    line();
    line(console::paint("     别人向东追日出，你向西赶日落。", console::CYAN));
    line("     —— 一个上海书大学生的十二周。");
    line();
    line("[1] 新学期开始    [2] 读取存档    [3] 玩法说明    [0] 退出");
    line();
    line(console::paint("（面向对象程序设计课程项目 · 垂直切片 demo）", console::DIM));
}

void TitleScreen::handleInput(const std::string& in) {
    std::string cmd = strutil::toLower(in);
    if (cmd == "1") game_.newGame();
    else if (cmd == "2") game_.screens().push(std::make_unique<SaveLoadScreen>(game_, false));
    else if (cmd == "3") game_.screens().push(std::make_unique<HelpScreen>(game_));
    else if (cmd == "0") game_.exitApp();
}

// ================= 校园（主界面） =================

void CampusScreen::draw() {
    GameContext* c = game_.ctx();
    if (!c) return;
    const LocationDef* loc = c->config.location(c->locationId);
    header("第 " + std::to_string(c->time->week()) + " 周 · " + c->time->dayName() +
           " · 行动点 " + std::to_string(c->time->actionsLeft()) + "/" +
           std::to_string(c->time->apPerWeek()));
    line("现金 " + console::paint(money(c->player->wallet().get()), console::GREEN) +
         " ｜ 平时分 " + std::to_string(c->player->studyScore()) +
         " ｜ 背包 " + std::to_string(c->player->inventory().usedSlots()) + "/" +
         std::to_string(c->player->inventory().capacity()) + " 格");
    line("当前地点：" + console::paint(loc ? loc->name : c->locationId, console::YELLOW) +
         "（目标：期末总资产 ≥ " + money(c->config.balance().targetMoney) + "，不挂科）");
    if (!c->rumors.empty()) {
        rule();
        for (const auto& r : c->rumors) line(console::paint(r, console::MAGENTA));
    }
    rule();
    if (!c->lastFeedback.empty()) line(console::paint(c->lastFeedback, console::CYAN));
    rule();
    std::string social = "[5]社交";
    if (loc && !loc->npcId.empty() && c->config.npc(loc->npcId))
        social = "[5]社交(" + c->config.npc(loc->npcId)->name + ")";
    line("[1]移动  [2]市场  [3]打工  [4]学习  " + social + "  [6]休息");
    line("[7]背包  [8]结束本周  [9]存档  [0]读档  [h]帮助  [q]回标题");
}

void CampusScreen::handleInput(const std::string& in) {
    std::string cmd = strutil::toLower(in);
    if (cmd == "1") game_.screens().push(std::make_unique<TravelScreen>(game_));
    else if (cmd == "2") game_.screens().push(std::make_unique<MarketScreen>(game_));
    else if (cmd == "3") game_.performAction(std::make_unique<WorkCommand>());
    else if (cmd == "4") game_.performAction(std::make_unique<StudyCommand>());
    else if (cmd == "5") game_.performAction(std::make_unique<SocialCommand>());
    else if (cmd == "6") game_.performAction(std::make_unique<RestCommand>());
    else if (cmd == "7") game_.screens().push(std::make_unique<InventoryScreen>(game_));
    else if (cmd == "8") game_.requestEndWeek();
    else if (cmd == "9") game_.screens().push(std::make_unique<SaveLoadScreen>(game_, true));
    else if (cmd == "0") game_.screens().push(std::make_unique<SaveLoadScreen>(game_, false));
    else if (cmd == "h") game_.screens().push(std::make_unique<HelpScreen>(game_));
    else if (cmd == "q") game_.backToTitle();
}

// ================= 移动 =================

void TravelScreen::draw() {
    GameContext* c = game_.ctx();
    if (!c) return;
    header("去哪儿？（移动耗 1 行动点 + 路费）");
    int i = 1;
    for (const auto& l : c->config.locations()) {
        std::string prefix = l.id == c->locationId ? "→" : " ";
        line(prefix + " [" + std::to_string(i) + "] " +
             strutil::padRight(l.name, 14) + "路费 ¥" + std::to_string(l.fare) +
             "  " + console::paint(l.desc, console::DIM));
        ++i;
    }
    rule();
    line("输入编号移动，[q] 返回");
}

void TravelScreen::handleInput(const std::string& in) {
    std::string cmd = strutil::toLower(in);
    GameContext* c = game_.ctx();
    if (!c) return;
    if (cmd == "q") {
        game_.screens().pop();
        return;
    }
    int idx = -1;
    try {
        idx = std::stoi(cmd);
    } catch (...) {
        return;
    }
    const auto& locs = c->config.locations();
    if (idx >= 1 && idx <= static_cast<int>(locs.size())) {
        game_.performAction(std::make_unique<MoveCommand>(locs[static_cast<std::size_t>(idx - 1)].id));
        game_.screens().pop();
    }
}

// ================= 市场 =================

void MarketScreen::draw() {
    GameContext* c = game_.ctx();
    if (!c) return;
    const LocationDef* loc = c->config.location(c->locationId);
    header("市场 · " + (loc ? loc->name : c->locationId) + " · 现金 " + money(c->player->wallet().get()));
    line(strutil::padRight("序号", 6) + strutil::padRight("商品", 14) +
         strutil::padRight("买价", 7) + strutil::padRight("卖价", 7) +
         strutil::padRight("持有", 6) + strutil::padRight("限购余", 7) + "趋势");
    rule();
    auto listings = c->market->listAt(c->locationId);
    int i = 1;
    for (const auto& li : listings) {
        std::string buy = li.canBuy ? std::to_string(li.buyPrice) : "—";
        std::string arrow = c->market->trendArrow(c->locationId, li.goods->id);
        const char* color = arrow[0] == 0xE2 && arrow[1] == 0x96
                                ? (arrow[2] == static_cast<char>(0xB2) ? console::RED : console::GREEN)
                                : console::DIM;
        line(strutil::padRight("[" + std::to_string(i) + "]", 6) +
             strutil::padRight(li.goods->name, 14) +
             strutil::padRight(buy, 7) +
             strutil::padRight(std::to_string(li.sellPrice), 7) +
             strutil::padRight(std::to_string(c->player->inventory().find(li.goods->id)
                                                  ? c->player->inventory().find(li.goods->id)->qty
                                                  : 0), 6) +
             strutil::padRight(li.canBuy ? std::to_string(li.quotaLeft) : "—", 7) +
             console::paint(arrow, color));
        ++i;
    }
    rule();
    line(console::paint(feedback_, console::CYAN));
    line("b <序号> [数量|max] 买入    s <序号> [数量|max] 卖出    q 返回（限购每日刷新）");
}

void MarketScreen::handleInput(const std::string& in) {
    GameContext* c = game_.ctx();
    if (!c) return;
    std::string cmd = strutil::toLower(in);
    if (cmd == "q" || cmd.empty()) {
        game_.screens().pop();
        return;
    }
    auto tokens = strutil::split(cmd);
    if (tokens.size() < 2) {
        feedback_ = "输入格式：b <序号> [数量|max] 或 s <序号> [数量|max]";
        return;
    }
    bool isBuy = tokens[0] == "b";
    bool isSell = tokens[0] == "s";
    if (!isBuy && !isSell) {
        feedback_ = "未知命令：" + tokens[0];
        return;
    }
    int idx = -1;
    try {
        idx = std::stoi(tokens[1]);
    } catch (...) {
        feedback_ = "序号要是数字";
        return;
    }
    auto listings = c->market->listAt(c->locationId);
    if (idx < 1 || idx > static_cast<int>(listings.size())) {
        feedback_ = "没有这个序号";
        return;
    }
    const Market::Listing& li = listings[static_cast<std::size_t>(idx - 1)];
    const std::string& goodsId = li.goods->id;

    int qty = 1;
    if (tokens.size() >= 3) {
        if (tokens[2] == "max") {
            if (isBuy) {
                int affordable = li.buyPrice > 0 ? c->player->wallet().get() / li.buyPrice : 0;
                int fit = c->player->inventory().freeSlots() / li.goods->slots;
                qty = std::min({li.quotaLeft, affordable, fit});
            } else {
                const Inventory::Item* item = c->player->inventory().find(goodsId);
                qty = item ? item->qty : 0;
            }
        } else {
            try {
                qty = std::stoi(tokens[2]);
            } catch (...) {
                feedback_ = "数量要是数字或 max";
                return;
            }
        }
    }
    TradeResult r = isBuy ? c->trade->buy(*c, goodsId, qty) : c->trade->sell(*c, goodsId, qty);
    feedback_ = r.msg;
}

// ================= 背包 =================

void InventoryScreen::draw() {
    GameContext* c = game_.ctx();
    if (!c) return;
    header("背包 · " + std::to_string(c->player->inventory().usedSlots()) + "/" +
           std::to_string(c->player->inventory().capacity()) + " 格");
    if (c->player->inventory().empty()) {
        line("背包空空如也。");
    } else {
        line(strutil::padRight("商品", 14) + strutil::padRight("数量", 6) +
             strutil::padRight("均价", 8) + strutil::padRight("本地卖价", 9) + "浮动盈亏");
        rule();
        for (const auto& item : c->player->inventory().items()) {
            const GoodsDef* g = c->config.goods(item.goodsId);
            if (!g) continue;
            int sellHere = c->market->sellPrice(c->locationId, item.goodsId);
            long long diff = static_cast<long long>(sellHere) * item.qty -
                             static_cast<long long>(item.avgCost) * item.qty;
            std::string diffStr = (diff >= 0 ? "+" : "") + std::to_string(diff);
            line(strutil::padRight(g->name, 14) +
                 strutil::padRight("x" + std::to_string(item.qty), 6) +
                 strutil::padRight(std::to_string(static_cast<int>(item.avgCost)), 8) +
                 strutil::padRight(std::to_string(sellHere), 9) +
                 console::paint(diffStr, diff >= 0 ? console::GREEN : console::RED));
        }
    }
    rule();
    line("[q] 返回（卖出请在市场界面操作）");
}

void InventoryScreen::handleInput(const std::string& in) {
    (void)in;
    game_.screens().pop(); // 任意键返回
}

// ================= 事件对话框 =================

void EventDialogScreen::draw() {
    if (!ev_) return;
    header("【" + ev_->name + "】");
    line(ev_->text);
    if (!resolved_) {
        rule();
        int i = 1;
        for (const auto& choice : ev_->choices)
            line("  [" + std::to_string(i++) + "] " + choice.text);
        rule();
        line("输入选项编号：");
    } else {
        rule();
        line(console::paint(resultText_, console::YELLOW));
        rule();
        line("[回车] 继续");
    }
}

void EventDialogScreen::handleInput(const std::string& in) {
    GameContext* c = game_.ctx();
    if (!c || !ev_) return;
    if (resolved_) {
        game_.screens().pop();
        return;
    }
    int idx = -1;
    try {
        idx = std::stoi(strutil::trim(in));
    } catch (...) {
        return; // 非数字输入忽略
    }
    if (idx >= 1 && idx <= static_cast<int>(ev_->choices.size())) {
        resultText_ = Session::applyChoice(*c, *ev_, idx - 1);
        resolved_ = true;
    }
}

// ================= 周报 =================

void WeeklyReportScreen::draw() {
    const auto& rep = game_.lastReport();
    header("周日结算 · 第 " + std::to_string(rep.week) + " 周结束");
    for (const auto& l : rep.lines) line("  · " + l);
    rule();
    line("现金：" + money(rep.moneyBefore) + " → " +
         console::paint(money(rep.moneyAfter), rep.moneyAfter >= rep.moneyBefore ? console::GREEN : console::RED));
    line("[回车] 进入新的一周");
}

void WeeklyReportScreen::handleInput(const std::string& in) {
    (void)in;
    game_.screens().pop();
}

// ================= 存读档 =================

void SaveLoadScreen::draw() {
    header(saveMode_ ? "保存进度" : "读取进度");
    for (const auto& s : game_.saves().listSlots()) {
        std::string label = s.slot == SaveManager::kAutoSlot ? "自动档" : "槽位 " + std::to_string(s.slot);
        line("[" + std::to_string(s.slot) + "] " + strutil::padRight(label, 10) +
             (s.exists ? console::paint(s.summary, console::GREEN) : console::paint("（空）", console::DIM)));
    }
    rule();
    if (!feedback_.empty()) line(console::paint(feedback_, console::CYAN));
    line("[0-3] 选择槽位    [q] 返回");
}

void SaveLoadScreen::handleInput(const std::string& in) {
    std::string cmd = strutil::toLower(in);
    if (cmd == "q") {
        game_.screens().pop();
        return;
    }
    if (cmd.size() != 1 || cmd[0] < '0' || cmd[0] > '3') return;
    int slot = cmd[0] - '0';
    std::string err;
    if (saveMode_) {
        feedback_ = game_.saveGame(slot, err) ? "已保存到槽位 " + std::to_string(slot)
                                              : "保存失败: " + err;
    } else {
        if (game_.loadGame(slot, err)) return; // 读档成功：屏幕栈已重置（本屏幕延迟销毁）
        feedback_ = "读档失败: " + err;
    }
}

// ================= 结局 =================

void EndingScreen::draw() {
    GameContext* c = game_.ctx();
    const EndingDef* e = game_.chosenEnding();
    if (!c || !e) return;
    header("结局 · 《" + e->title + "》");
    line();
    line(e->text);
    line();
    rule();
    int portfolio = c->trade->estimatePortfolioValue(*c);
    line("期末总资产（现金+库存估值）：" + money(c->player->wallet().get() + portfolio) +
         "（目标 " + money(c->config.balance().targetMoney) + "）");
    line("平时分：" + std::to_string(c->player->studyScore()) +
         (c->flags->has("failed") ? "（挂科）" : "（未挂科）"));
    line("好感：室友 " + std::to_string(c->npcs->affinity("roommate")) +
         " ｜ 学长 " + std::to_string(c->npcs->affinity("senior")) +
         " ｜ 学姐 " + std::to_string(c->npcs->affinity("xuejie")));
    line("本学期交易笔数：" + std::to_string(c->player->ledger().size()));
    rule();
    line("[回车] 回到标题");
}

void EndingScreen::handleInput(const std::string& in) {
    (void)in;
    game_.backToTitle();
}

// ================= 帮助 =================

void HelpScreen::draw() {
    header("玩法说明");
    line("· 一学期 12 周，每周 6 个行动点（周一~周六，1 行动点 = 1 天），周日自动结算。");
    line("· 移动/打工/学习/社交/休息各耗 1 点；同一地点内买卖不耗点。");
    line("· 低买高卖赚差价：留意地点系数、每周趋势和需求高峰（考试周教材疯涨）。");
    line("· 每周生活费自动扣除；期末总资产 ≥ ¥12000 + 不挂科 + 一位好友 → 好结局。");
    line("· 社交线：好感 30/60/80 解锁情报、折扣、内部票（先去认识驻地 NPC）。");
    line("· 情报来自室友，每周一条，80% 准确 —— 剩下的 20% 就是人生。");
    line();
    line("[任意键] 返回");
}

void HelpScreen::handleInput(const std::string& in) {
    (void)in;
    game_.screens().pop();
}
