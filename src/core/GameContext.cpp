#include "core/GameContext.h"

#include "core/ConfigManager.h"
#include "core/DomainEvents.h"
#include "core/EventBus.h"
#include "core/Random.h"
#include "economy/Market.h"
#include "events/EventScheduler.h"
#include "player/Player.h"
#include "player/TradeSystem.h"
#include "story/NpcSystem.h"
#include "story/StoryFlags.h"
#include "stock/StockSystem.h"
#include "time/TimeSystem.h"
#include "util/JsonUtil.h"
#include "core/ITimeListener.h"

GameContext::GameContext(const ConfigManager& config, Random& rng, EventBus& bus)
    : config(config), rng(rng), bus(bus) {
    // ---- 组合根：所有系统在这里组装（分工 7.2 GameContext 职责） ----
    player = std::make_unique<Player>(config);
    market = std::make_unique<Market>(config, rng);
    time = std::make_unique<TimeSystem>(config);
    events = std::make_unique<EventScheduler>(config, rng);
    flags = std::make_unique<StoryFlags>();
    npcs = std::make_unique<NpcSystem>(config);
    trade = std::make_unique<TradeSystem>(config);
    stock = std::make_unique<StockSystem>(config, rng);

    events->buildFromConfig(); // 事件表随配置构建（新局/读档共用）

    wireSubscriptions();
}

GameContext::~GameContext() {
    // 摘掉本上下文注册的订阅：Player 随上下文析构，留着订阅就是悬垂指针
    for (const std::uint64_t id : subscriptionIds_) bus.unsubscribe(id);
    subscriptionIds_.clear();
}

// 破产判定：钱包被炒股债务扣到破产线以下
bool GameContext::isBankrupt() const {
    return player->wallet().get() < -config.balance().bankruptcyDebt;
}

void GameContext::updateBankruptcyFlag() {
    if (isBankrupt()) flags->set("bankrupt");
}

// 昏迷/宵禁统一入口：任何行动（含屏幕上的免费交易）把时间推过凌晨 2 点都会走到这里
bool GameContext::handleFaintAndAdvance(std::string& appendMsg) {
    if (time->hour() < TimeSystem::kCurfewHour) return false;

    const BalanceDef& b = config.balance();
    const int fee = std::min(player->wallet().get(), b.faintHospitalFee);
    if (fee > 0) player->wallet().spend(fee);
    player->energy().reduceMax(b.faintMaxStaminaLoss);
    const bool weekAdvanced = time->advanceDay();
    locationId = "dorm"; // 被室友抬回宿舍
    ITimeListener& marketListener = *market;
    marketListener.onDayTick(); // 新的一天：限购/噪声刷新
    appendMsg += "\n【昏迷】凌晨 2 点，你眼前一黑倒在原地……醒来时已躺在校医院"
                 "（诊疗费 -¥" + std::to_string(fee) +
                 "，体力上限 -" + std::to_string(b.faintMaxStaminaLoss) + "）";
    updateBankruptcyFlag();
    return weekAdvanced;
}

// Observer 接线：交易完成 → 玩家自动记账（订阅方只依赖事件，不依赖发布方）
void GameContext::wireSubscriptions() {
    Player* p = player.get();
    subscriptionIds_.push_back(
        bus.subscribe<TradeCompletedEvent>([p](const TradeCompletedEvent& e) {
            p->onTradeCompleted(e);
        }));
}

void GameContext::initNewGame() {
    player->wallet().add(config.balance().startMoney);
    market->syncWeek(1);
    market->onWeekStart(); // 第 1 周趋势
    market->rollRumor();   // 生成对第 2 周的情报
    rumors = market->rumorTexts();
    bus.publish(WeekStartedEvent{1});
}

Json GameContext::toJson() const {
    return Json{
        {"meta", Json{{"location", locationId}, {"rumors", rumors}}},
        {"player", player->toJson()},
        {"market", market->toJson()},
        {"stock", stock->toJson()},
        {"time", time->toJson()},
        {"events", events->toJson()},
        {"flags", flags->toJson()},
        {"npcs", npcs->toJson()},
    };
}

void GameContext::loadFromJson(const Json& save) {
    if (save.contains("meta")) {
        locationId = ju::getOr<std::string>(save["meta"], "location", locationId);
        if (!config.location(locationId)) locationId = "dorm"; // 坏档兜底
        rumors.clear();
        if (save["meta"].contains("rumors") && save["meta"]["rumors"].is_array())
            for (const auto& r : save["meta"]["rumors"]) rumors.push_back(r.get<std::string>());
    }
    if (save.contains("player")) player->fromJson(save["player"]);
    if (save.contains("market")) market->fromJson(save["market"]);
    if (save.contains("stock")) stock->fromJson(save["stock"]);
    if (save.contains("time")) time->fromJson(save["time"]);
    if (save.contains("events")) events->fromJson(save["events"]);
    if (save.contains("flags")) flags->fromJson(save["flags"]);
    if (save.contains("npcs")) npcs->fromJson(save["npcs"]);
}
