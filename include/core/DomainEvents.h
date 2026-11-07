#pragma once
// ============================================================
// [core/DomainEvents] 领域事件载荷（组长 A 维护）
// 这些结构体是 EventBus 上流动的"消息"，订阅方只依赖消息本身，
// 不依赖发布方 —— 这就是 Observer 模式解耦的关键。
// ============================================================
#include <string>

// 一笔交易成交（发布者：TradeSystem；订阅者：玩家账本、日后 UI 刷新）
struct TradeCompletedEvent {
    std::string goodsId;
    int qty = 0;
    int total = 0;     // 总金额（买为支出、卖为收入）
    bool isBuy = false;
};

// 新的一周开始（发布者：WeekSettlement；订阅者：日后 UI 刷新行情/周报）
struct WeekStartedEvent {
    int week = 1;
};

// 结局已判定（发布者：Game；订阅者：结局画面、统计）
struct EndingDecidedEvent {
    std::string endingId;
    std::string title;
};

// 一次行动完成（发布者：Session；订阅者：日后统计/回放）
struct ActionTakenEvent {
    std::string actionName;
};
