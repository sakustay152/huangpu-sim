#pragma once
// ============================================================
// [player/TradeError] 交易失败原因（组员 C）
// 分工 7.4：交易失败用返回值表达，不用异常做流程控制。
// ============================================================
enum class TradeError {
    None = 0,
    InvalidCount,    // 数量 <= 0
    UnknownGoods,    // 没有这个商品
    WrongLocation,   // 这个地点不卖它（买入时）
    NotEnoughCash,   // 现金不足
    NotEnoughSlot,   // 背包放不下
    QuotaExceeded,   // 超过当日限购
    NotOwned,        // 卖出时没有这么多货
};
