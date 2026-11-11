#include "core/Game.h"

#include <iostream>

#include "core/GameContext.h"
#include "core/ITimeListener.h"
#include "core/Session.h"
#include "economy/Market.h"
#include "events/EndingJudge.h"
#include "events/EventScheduler.h"
#include "events/GameEvent.h"
#include "player/Player.h"
#include "stock/StockSystem.h"
#include "story/StoryFlags.h"
#include "time/TimeSystem.h"
#include "util/JsonUtil.h"
#include "util/StringUtils.h"
#include "ui/Screens.h"

Game::Game(const std::string& dataDir, const std::string& saveDir, std::uint64_t seed)
    : config_(dataDir), rng_(seed), saves_(saveDir) {}

Game::~Game() = default;

std::string Game::readLine() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        running_ = false; // EOF：干净退出
        return "";
    }
    return strutil::trim(line);
}

void Game::pushScreen(std::unique_ptr<IScreen> screen) {
    screens_.push(std::move(screen));
}

void Game::resolveFlow() {
    screens_.flushGraveyard(); // 销毁上一次输入处理中弹出的屏幕
    if (!ctx_) {
        if (screens_.empty()) pushScreen(std::make_unique<TitleScreen>(*this));
        return;
    }
    // 待弹窗（周报/事件）依序入栈；模态弹窗显示期间等待
    while (!pendingDialogs_.empty()) {
        if (screens_.topIsModal()) break;
        pushScreen(std::move(pendingDialogs_.front()));
        pendingDialogs_.pop_front();
    }
    if (screens_.empty()) {
        if (endingPending_) {
            endingPending_ = false;
            pushScreen(std::make_unique<EndingScreen>(*this));
        } else {
            pushScreen(std::make_unique<CampusScreen>(*this));
        }
    }
}

void Game::run() {
    while (running_) {
        resolveFlow();
        IScreen* top = screens_.top();
        if (!top) break;
        top->render();
        std::string input = readLine();
        if (!running_) break;
        top->handleInput(input);
    }
}

// ---------------- 流程控制 ----------------

void Game::newGame() {
    ctx_ = std::make_unique<GameContext>(config_, rng_, bus_);
    ctx_->initNewGame();
    screens_.popAll();
    pendingDialogs_.clear();
    endingPending_ = false;
    chosenEnding_ = nullptr;
    ctx_->lastFeedback = "开学了。启动资金 ¥" + std::to_string(config_.balance().startMoney) +
                         "，这学期不再向家里要钱。";
    // 第 1 周固定节点（开学事件）入队
    for (const GameEvent* ev : ctx_->events->fixedForWeek(1, *ctx_)) queueEventDialog(ev);
}

bool Game::loadGame(int slot, std::string& err) {
    Json data = saves_.load(slot, &err);
    if (data.is_null()) return false;
    try {
        if (!ctx_) ctx_ = std::make_unique<GameContext>(config_, rng_, bus_);
        ctx_->loadFromJson(data);
        if (data.contains("rng")) rng_.fromJson(data["rng"]);
        screens_.popAll();
        pendingDialogs_.clear();
        endingPending_ = false;
        chosenEnding_ = nullptr;
        return true;
    } catch (const std::exception& e) {
        err = std::string("读档失败: ") + e.what();
        return false;
    }
}

bool Game::saveGame(int slot, std::string& err) {
    if (!ctx_) {
        err = "当前没有进行中的游戏";
        return false;
    }
    Json data = ctx_->toJson();
    data["version"] = 1;
    data["rng"] = rng_.toJson();
    data["meta"]["week"] = ctx_->time->week();
    data["meta"]["money"] = ctx_->player->wallet().get();
    return saves_.save(slot, data, &err);
}

void Game::performAction(std::unique_ptr<ICommand> cmd) {
    GameContext* c = ctx_.get();
    if (!c || !cmd) return;
    ActionOutcome out = Session::runAction(*c, *cmd);
    c->lastFeedback = out.result.msg;
    if (out.event) queueEventDialog(out.event);
    checkBankruptcy(); // 行动中的股价波动可能触发爆仓欠债
    // 跨越周日 → 自动进入周结算
    if (out.weekAdvanced && !c->time->isSemesterOver()) {
        requestEndWeek();
    }
}

// 炒股爆仓欠债超过破产线 → 立即终止本学期，进入《破产清算》结局
void Game::checkBankruptcy() {
    GameContext* c = ctx_.get();
    if (!c || endingPending_ || chosenEnding_) return;
    if (c->flags->has("bankrupt") && !c->time->isSemesterOver()) {
        chosenEnding_ = EndingJudge::judge(*c); // priority=0 的破产结局优先生效
        EndingDecidedEvent e{chosenEnding_->id, chosenEnding_->title};
        bus().publish(e);
        endingPending_ = true;
        pendingDialogs_.clear();
        screens_.popAll(); // 当前界面全部退役，resolveFlow 会推入结局屏
    }
}

// 临时调试：直接跳到结局（验证结局→重开流程用）
void Game::debugJumpToEnding() {
    GameContext* c = ctx_.get();
    if (!c || endingPending_ || chosenEnding_) return;
    chosenEnding_ = EndingJudge::judge(*c);
    EndingDecidedEvent e{chosenEnding_->id, chosenEnding_->title};
    bus().publish(e);
    endingPending_ = true;
    pendingDialogs_.clear();
    screens_.popAll();
}

// [8] 快进到周日结算：剩余天数逐日睡过（体力回满），然后进入周结算
void Game::finishWeekNow() {
    GameContext* c = ctx_.get();
    if (!c || endingPending_ || c->time->isSemesterOver()) return;
    while (true) {
        const bool weekRolled = c->time->advanceDay();
        c->player->energy().restoreFull(); // 睡满一整天
        ITimeListener& marketListener = *c->market;
        marketListener.onDayTick();
        c->stock->onTick(*c); // 跳过的天 K 线也照走
        if (weekRolled || c->time->isSemesterOver()) break;
    }
    checkBankruptcy();
    requestEndWeek();
}

void Game::requestEndWeek() {
    GameContext* c = ctx_.get();
    if (!c || endingPending_) return;

    WeekSettlement settle(*c);
    lastReport_ = settle.run();

    // 每周日自动存档（分工 7.2）
    std::string err;
    saveGame(SaveManager::kAutoSlot, err);

    if (c->flags->has("bankrupt")) {
        // 本周爆仓欠债超过破产线：先看结算报告，关闭后直接进破产结局
        chosenEnding_ = EndingJudge::judge(*c);
        bus().publish(EndingDecidedEvent{chosenEnding_->id, chosenEnding_->title});
        endingPending_ = true;
        queueWeeklyReport();
        return;
    }

    queueWeeklyReport(); // 周报最先弹出
    std::cerr << "[dbg] requestEndWeek: queued report, pending=" << pendingDialogs_.size()
              << " week=" << c->time->week() << std::endl;
    if (c->time->isSemesterOver()) {
        chosenEnding_ = EndingJudge::judge(*c);
        bus().publish(EndingDecidedEvent{chosenEnding_->id, chosenEnding_->title});
        endingPending_ = true;
        screens_.popAll(); // 校园退役，周报关闭后直接进结局
    } else {
        // 新一周的固定节点（期中/双十一/期末…）依序入队
        for (const GameEvent* ev : c->events->fixedForWeek(c->time->week(), *c))
            queueEventDialog(ev);
    }
}

void Game::backToTitle() {
    ctx_.reset();
    pendingDialogs_.clear();
    endingPending_ = false;
    chosenEnding_ = nullptr;
    screens_.popAll();
}

void Game::queueEventDialog(const GameEvent* ev) {
    if (ev) pendingDialogs_.push_back(std::make_unique<EventDialogScreen>(*this, ev));
}

void Game::queueWeeklyReport() {
    pendingDialogs_.push_back(std::make_unique<WeeklyReportScreen>(*this));
}
