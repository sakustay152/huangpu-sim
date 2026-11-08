#include "ui/Screens.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>

#include "actions/Commands.h"
#include "core/ConfigManager.h"
#include "core/Game.h"
#include "core/GameContext.h"
#include "core/IEffect.h"
#include "core/Session.h"
#include "economy/Market.h"
#include "events/GameEvent.h"
#include "player/Inventory.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "stock/StockSystem.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"
#include "util/Console.h"
#include "util/StringUtils.h"

namespace {

std::string money(int v) { return "¥" + std::to_string(v); }

std::string money(double v) { return "¥" + std::to_string(static_cast<long long>(v)); }

std::string fmtPrice(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}

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
         " ｜ 体力 " + std::to_string(c->player->energy().get()) + "/" +
         std::to_string(c->player->energy().max()) +
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
    line("[1]移动  [2]市场  [3]打工  [4]学习  " + social + "  [6]休息  [x]炒股");
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
    else if (cmd == "x") game_.screens().push(std::make_unique<StockScreen>(game_));
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

// ================= 炒股（bilibili fx-simple 风：K 线 + 鼠标按钮） =================

namespace {
constexpr int kChartRows = 12; // K 线图行数
constexpr int kChartBars = 18; // 可见 K 线条数
constexpr int kBtnRow1 = 24;   // 按钮行 1（draw 的输出行号）
constexpr int kBtnRow2 = 25;   // 按钮行 2

// 蜡烛图渲染：红涨绿跌（A股习惯），最右侧标注现价
void drawCandles(const std::vector<StockBar>& bars, double lastPrice) {
    if (bars.empty()) {
        std::cout << "（暂无行情）\n";
        return;
    }
    std::size_t start = bars.size() > static_cast<std::size_t>(kChartBars)
                            ? bars.size() - static_cast<std::size_t>(kChartBars)
                            : 0;
    double lo = bars[start].l, hi = bars[start].h;
    for (std::size_t i = start; i < bars.size(); ++i) {
        lo = std::min(lo, bars[i].l);
        hi = std::max(hi, bars[i].h);
    }
    if (hi - lo < 1e-9) hi = lo + 1.0;
    double rowSpan = (hi - lo) / kChartRows;
    for (int r = 0; r < kChartRows; ++r) {
        double pTop = hi - (hi - lo) * r / kChartRows;
        double pBot = hi - (hi - lo) * (r + 1) / kChartRows;
        std::string rowStr;
        for (std::size_t i = start; i < bars.size(); ++i) {
            const StockBar& b = bars[i];
            double bodyLo = std::min(b.o, b.c), bodyHi = std::max(b.o, b.c);
            bool up = b.c >= b.o;
            const char* col = up ? console::RED : console::GREEN;
            bool bodyHit;
            if (bodyHi - bodyLo >= rowSpan) bodyHit = bodyLo < pTop && bodyHi > pBot;
            else bodyHit = (bodyLo + bodyHi) / 2 >= pBot && (bodyLo + bodyHi) / 2 < pTop;
            bool wickHit = (b.l < pTop && b.h > pBot) && !bodyHit;
            if (bodyHit) rowStr += console::paint("███", col);
            else if (wickHit) rowStr += console::paint(" │ ", col);
            else rowStr += "   ";
            rowStr += " ";
        }
        if (lastPrice >= pBot && lastPrice <= pTop)
            rowStr += console::paint("← 现价 ¥" + fmtPrice(lastPrice), console::YELLOW);
        std::cout << rowStr << "\n";
    }
}

int bestChoiceForMoney(const GameEvent& ev, const GameContext& ctx) {
    (void)ctx;
    int best = 0;
    long long bv = std::numeric_limits<long long>::min();
    for (std::size_t i = 0; i < ev.choices.size(); ++i) {
        long long v = 0;
        for (const auto& e : ev.choices[i].effects) v += e->estimateMoneyDelta();
        if (v > bv) {
            bv = v;
            best = static_cast<int>(i);
        }
    }
    return best;
}

std::string flatten(std::string s) {
    std::size_t pos = 0;
    while ((pos = s.find("\n", pos)) != std::string::npos) {
        s.replace(pos, 1, "；");
        pos += 3;
    }
    return s;
}
} // namespace

// 进入炒股屏：真实控制台 → 接管输入循环（鼠标点击按钮 / 快捷键）；
// 输入被重定向（脚本化回归）→ 回退到键盘行命令模式
void StockScreen::onEnter() {
    if (!console::stdinIsConsole()) {
        lineFallback_ = true;
        return;
    }
    console::InteractiveGuard guard; // 鼠标模式 + 隐藏光标，析构恢复
    bool running = true;
    render();
    while (running) {
        console::ConsoleEvent ev;
        if (!console::pollEvent(100, ev)) continue;
        GameContext* c = game_.ctx();
        if (!c || game_.screens().top() != this) return; // 流程已接管（破产/结局）
        bool changed = false;
        if (ev.mouse && ev.clicked) {
            for (auto& b : buttons_) {
                if (ev.y == b.row && ev.x >= b.c0 && ev.x <= b.c1) {
                    b.action();
                    changed = true;
                    break;
                }
            }
            if (!changed) continue; // 点在空白处不重绘
        } else if (ev.has) { // 键盘
            if (ev.key == 'q' || ev.key == 27) running = false;
            else if (ev.key >= '1' && ev.key <= '6') {
                int idx = ev.key - '1';
                if (idx < static_cast<int>(buttons_.size())) {
                    buttons_[static_cast<std::size_t>(idx)].action();
                    changed = true;
                }
            }
        }
        if (!c || game_.screens().top() != this) return;
        if (changed) render();
        if (c->flags->has("bankrupt")) return; // 爆仓 → 结局流程接管
        if (!c->time->canAct() && !c->time->isSemesterOver()) {
            game_.requestEndWeek(); // 学股市知识耗尽行动点 → 周日结算
            running = false;
        }
    }
    game_.screens().pop(); // 自弹栈（延迟销毁保证安全）
}

void StockScreen::draw() {
    GameContext* c = game_.ctx();
    if (!c) return;
    const StockSystem* s = c->stock.get();
    header("浦江文旅（SHU）· 炒股模拟器 · 股力 " +
           std::to_string(c->player->stockSkill()) + "/" +
           std::to_string(c->config.balance().stockSkillMax) +
           " · 体力 " + std::to_string(c->player->energy().get()) + "/" +
           std::to_string(c->player->energy().max()));

    // 现价与涨跌（对前一根 K 线收盘）
    const auto& bars = s->bars();
    double prev = bars.size() >= 2 ? bars[bars.size() - 2].c : s->price();
    double pct = prev > 1e-9 ? (s->price() - prev) / prev * 100.0 : 0.0;
    char pctBuf[32];
    std::snprintf(pctBuf, sizeof(pctBuf), "%+.2f%%", pct);
    line(console::paint(std::string("现价 ¥") + fmtPrice(s->price()) + "  " + pctBuf,
                        pct >= 0 ? console::RED : console::GREEN) +
         " ｜ 持仓时收阴概率 " +
         console::paint(std::to_string(static_cast<int>(std::round(s->effectiveFailProb(*c) * 100))) + "%",
                        console::YELLOW) +
         "（股力越高越倾向赚钱，但永不保证）");

    drawCandles(bars, s->price());
    rule();
    if (!s->notice().empty()) line(console::paint(s->notice(), console::RED));
    line("持仓 " + std::to_string(s->shares()) + " 股（成本 ¥" + fmtPrice(s->avgCost()) + "/股）"
         " ｜ 市值 " + money(s->positionValue()) +
         " ｜ 浮动盈亏 " + console::paint((s->unrealizedPnl() >= 0 ? "+" : "") + std::to_string(static_cast<long long>(s->unrealizedPnl())),
                                          s->unrealizedPnl() >= 0 ? console::RED : console::GREEN));
    line("融资负债 " + console::paint(money(static_cast<double>(s->loan())),
                                       s->loan() > 0 ? console::RED : console::DIM) +
         " ｜ 净持仓 " + money(s->equity()) + " ｜ 杠杆 x" + std::to_string(s->leverage()));
    line("现金 " + money(c->player->wallet().get()) +
         " ｜ 破产线：欠债 ≥ " + money(static_cast<double>(c->config.balance().bankruptcyDebt)));
    rule();
    line(console::paint(feedback_.empty() ? "欢迎来到 K 线的世界。" : feedback_, console::CYAN));
    rule();

    // ---- 鼠标按钮（按行列命中） ----
    buttons_.clear();
    const auto& opts = s->leverageOptions();
    int li = 0;
    for (int i = 0; i < static_cast<int>(opts.size()); ++i)
        if (opts[i] == s->leverage()) li = i;
    int nextLev = opts[(li + 1) % opts.size()];
    auto drawButtonRow = [&](int row, const std::vector<std::pair<std::string, std::function<void()>>>& defs) {
        int col = 2;
        for (const auto& [label, act] : defs) {
            int w = static_cast<int>(strutil::displayWidth(label));
            buttons_.push_back(Button{row, col, col + w - 1, act});
            std::cout << console::paint(label, console::CYAN);
            col += w + 3;
        }
        std::cout << "\n";
    };
    drawButtonRow(kBtnRow1, {
        {"[1:买入100]", [this] { clickBuy(100); }},
        {"[2:买入1000]", [this] { clickBuy(1000); }},
        {"[3:买入All]", [this] { clickBuy(game_.ctx()->player->wallet().get()); }},
        {"[4:清仓]", [this] { clickSellAll(); }},
    });
    drawButtonRow(kBtnRow2, {
        {"[5:杠杆 x" + std::to_string(s->leverage()) + "→x" + std::to_string(nextLev) + "]",
         [this] { clickCycleLeverage(); }},
        {"[6:学股市知识 -" + std::to_string(c->config.balance().learnStockStaminaCost) + "体力]",
         [this] { clickLearn(); }},
        {"[q:返回]", [this] {}},
    });
    line(console::paint("鼠标点击按钮或按快捷键 1-6；ESC/q 返回（K 线随每次行动与周日结算刷新）",
                        console::DIM));
}

void StockScreen::clickBuy(int yuan) {
    GameContext* c = game_.ctx();
    if (!c) return;
    TradeResult r = c->stock->buy(*c, yuan);
    feedback_ = r.msg;
    game_.checkBankruptcy();
}

void StockScreen::clickSellAll() {
    GameContext* c = game_.ctx();
    if (!c) return;
    TradeResult r = c->stock->sellAll(*c);
    feedback_ = r.msg;
    game_.checkBankruptcy();
}

void StockScreen::clickCycleLeverage() {
    GameContext* c = game_.ctx();
    if (!c) return;
    const auto& opts = c->stock->leverageOptions();
    int cur = c->stock->leverage();
    int idx = 0;
    for (int i = 0; i < static_cast<int>(opts.size()); ++i)
        if (opts[i] == cur) idx = i;
    TradeResult r = c->stock->setLeverage(opts[(idx + 1) % opts.size()]);
    feedback_ = r.msg;
}

void StockScreen::clickLearn() {
    GameContext* c = game_.ctx();
    if (!c) return;
    ActionOutcome out = Session::runAction(*c, LearnStockCommand());
    c->lastFeedback = out.result.msg;
    if (out.event) {
        // 炒股屏内触发随机事件：按"对钱最有利"立即解决，结果并入反馈行
        std::string msg = Session::applyChoice(*c, *out.event,
                                               bestChoiceForMoney(*out.event, *c));
        feedback_ = "【" + out.event->name + "】" + flatten(msg);
    } else {
        feedback_ = out.result.msg;
    }
    game_.checkBankruptcy();
    if (out.result.consumedAp && c->time->actionsLeft() == 0 && !c->time->isSemesterOver())
        game_.requestEndWeek(); // 行动点耗尽：排队周报，退出炒股屏后弹出
}

void StockScreen::handleInput(const std::string& in) {
    // 键盘回退模式（输入被重定向时）；交互模式由 onEnter 内循环处理
    GameContext* c = game_.ctx();
    if (!c) return;
    std::string cmd = strutil::toLower(strutil::trim(in));
    if (cmd == "q" || cmd.empty()) {
        game_.screens().pop();
        return;
    }
    auto tokens = strutil::split(cmd);
    if (tokens.empty()) return;
    if (tokens[0] == "b" && tokens.size() >= 2) {
        int yuan = 0;
        if (tokens[1] == "all") yuan = c->player->wallet().get();
        else {
            try {
                yuan = std::stoi(tokens[1]);
            } catch (...) {
                feedback_ = "金额要是数字或 all";
                return;
            }
        }
        clickBuy(yuan);
        return;
    }
    if (tokens[0] == "s") {
        clickSellAll();
        return;
    }
    if (tokens[0] == "l") {
        clickCycleLeverage();
        return;
    }
    if (tokens[0] == "learn") {
        clickLearn();
        if (c->time->actionsLeft() == 0 && !c->time->isSemesterOver())
            game_.screens().pop(); // 行动点耗尽：弹出排队周报
        return;
    }
    feedback_ = "命令：b <金额|all> / s all / l 切杠杆 / learn 学股市知识 / q 返回";
}

// ================= 事件对话框 =================

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
    if (c->flags->has("bankrupt"))
        line(console::paint("债务状况：欠债 ¥" + std::to_string(-c->player->wallet().get()) +
                                " —— 征信已破产，本项目提前终止",
                            console::RED));
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
    line("· 一学期 16 个教学周 + 第 17、18 周考试周（共 18 周），每周 6 个行动点（1 点 = 1 天），周日结算。");
    line("· 移动/打工/学习/社交/休息各耗 1 点；同一地点内买卖不耗点。");
    line("· 炒股：手机随时买卖「浦江文旅（SHU）」，耗体力不耗行动点；可上杠杆融资。");
    line("  新手胜率只有 ~38%，爆仓会欠债——欠债超过 ¥3000 触发《破产清算》结局。");
    line("· 低买高卖赚差价：留意地点系数、每周趋势和需求高峰（期中第 8 周、考试季 15~18 周）。");
    line("· 考试周学习有冲刺加成（图书馆 +9 分），教材/咖啡/打印额度价格飞涨。");
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
