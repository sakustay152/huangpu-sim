#pragma once
// ============================================================
// [economy/Market] 市场与行情（组员 B 主责；A 集成最小版）
// 职责：每个地点每种商品的买价/卖价、周趋势、日噪声、事件冲击、
//       每周限购、情报（rumor）。实现 ISaveable + ITimeListener。
// 价格计算委托 IPriceModel 装饰器链（策略+装饰器模式）。
// ============================================================
#include <map>
#include <string>
#include <vector>

#include "core/ConfigDefs.h"
#include "core/ISaveable.h"
#include "core/ITimeListener.h"

class Random;
class IPriceModel;
class ConfigManager;

class Market : public ISaveable, public ITimeListener {
public:
    Market(const ConfigManager& cfg, Random& rng);
    ~Market();
    Market(const Market&) = delete;
    Market& operator=(const Market&) = delete;

    struct Listing {
        const GoodsDef* goods = nullptr;
        int buyPrice = 0;
        int sellPrice = 0;
        int quotaLeft = 0;   // 本周在该地点剩余限购
        bool canBuy = false; // 该地点是否有售
    };

    // 某地点的可交易清单（全部商品都列：可买/只能卖一目了然）
    std::vector<Listing> listAt(const std::string& locId) const;

    int buyPrice(const std::string& locId, const std::string& goodsId) const;
    int sellPrice(const std::string& locId, const std::string& goodsId) const;

    int quotaLeft(const std::string& locId, const std::string& goodsId) const;
    bool consumeQuota(const std::string& locId, const std::string& goodsId, int qty);

    // 事件冲击（EventShockEffect 调用）
    void applyShock(const std::string& goodsId, double mult, int days);

    // 趋势箭头（UI 用）：▲ ▼ ─
    const char* trendArrow(const std::string& locId, const std::string& goodsId) const;

    // ---- ITimeListener ----
    void onDayTick() override;   // 噪声重摇 + 冲击衰减 + 限购重置（每次到访=1天）
    void onWeekStart() override; // 周趋势刷新（消化上周情报）

    // 周开始后调用：为新的一周设定当前周数（季节因子需要）
    void syncWeek(int week) { week_ = week; }

    // 摇一条下周情报（周结算时调用）；文本用 rumorTexts() 取
    void rollRumor();
    const std::vector<std::string>& rumorTexts() const { return rumorTexts_; }

    // ---- ISaveable ----
    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    struct RumorSpec {
        std::string goodsId;
        bool up = true;
    };

    double midPrice(const std::string& locId, const std::string& goodsId) const;
    double trendOf(const std::string& locId, const std::string& goodsId) const;
    double noiseOf(const std::string& goodsId) const;
    double shockOf(const std::string& goodsId) const;
    void resetQuotas();

    const ConfigManager& cfg_;
    Random& rng_;
    std::unique_ptr<IPriceModel> chain_; // 价格因子链（策略+装饰器）
    int week_ = 1;

    std::map<std::string, std::map<std::string, double>> trend_; // loc→goods→trend
    std::map<std::string, double> noiseMult_;                    // goods→乘数
    std::map<std::string, std::pair<double, int>> shock_;        // goods→{乘数,剩余天数}
    std::map<std::string, std::map<std::string, int>> quota_;    // loc→goods→剩余限购

    RumorSpec pendingRumor_;            // 对"下一周"的情报
    bool hasRumor_ = false;
    std::vector<std::string> rumorTexts_;
};
