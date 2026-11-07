#pragma once
// ============================================================
// [events/Effect] 事件效果（D 主责的机制；A 集成最小版）
// 分工 7.5：效果数据化（JSON 描述 → EffectFactory 创建），
// 不给每个事件写一段代码。IfEffect 是组合效果（Composite 模式）。
// ============================================================
#include <memory>
#include <string>
#include <vector>

#include "core/IEffect.h"
#include "util/Json.h"

class ICondition;

// 现金变动（负数为支出；现金不足时花光为止，不产生负债）
class MoneyEffect : public IEffect {
public:
    explicit MoneyEffect(int amount) : amount_(amount) {}
    std::string apply(GameContext& ctx) const override;
    int estimateMoneyDelta() const override { return amount_; }
private:
    int amount_;
};

// 随机现金（摆摊/捡钱类）
class RandomMoneyEffect : public IEffect {
public:
    RandomMoneyEffect(int min, int max) : min_(min), max_(max) {}
    std::string apply(GameContext& ctx) const override;
    int estimateMoneyDelta() const override { return (min_ + max_) / 2; }
private:
    int min_, max_;
};

// NPC 好感变动（跨过 30/60/80 门槛时附加解锁提示）
class AffinityEffect : public IEffect {
public:
    AffinityEffect(std::string npcId, int delta) : npcId_(std::move(npcId)), delta_(delta) {}
    std::string apply(GameContext& ctx) const override;
private:
    std::string npcId_;
    int delta_;
};

// 设置剧情 flag
class FlagEffect : public IEffect {
public:
    explicit FlagEffect(std::string flag) : flag_(std::move(flag)) {}
    std::string apply(GameContext& ctx) const override;
private:
    std::string flag_;
};

// 商品价格冲击（暴涨/暴跌，按天衰减）
class PriceShockEffect : public IEffect {
public:
    PriceShockEffect(std::string goodsId, double mult, int days)
        : goodsId_(std::move(goodsId)), mult_(mult), days_(days) {}
    std::string apply(GameContext& ctx) const override;
private:
    std::string goodsId_;
    double mult_;
    int days_;
};

// 直接获得商品（免费入包，放不下则部分放弃）
class ItemEffect : public IEffect {
public:
    ItemEffect(std::string goodsId, int qty) : goodsId_(std::move(goodsId)), qty_(qty) {}
    std::string apply(GameContext& ctx) const override;
    int estimateMoneyDelta() const override;
private:
    std::string goodsId_;
    int qty_;
};

// 平时分变动
class StudyEffect : public IEffect {
public:
    explicit StudyEffect(int delta) : delta_(delta) {}
    std::string apply(GameContext& ctx) const override;
private:
    int delta_;
};

// 延迟效果（例：借室友 500，3 周后还 600）
class ScheduledEffect : public IEffect {
public:
    ScheduledEffect(int weeks, Json effectJson)
        : weeks_(weeks), effectJson_(std::move(effectJson)) {}
    std::string apply(GameContext& ctx) const override;
private:
    int weeks_;
    Json effectJson_;
};

// 条件效果（Composite）：条件成立才应用内部效果
class IfEffect : public IEffect {
public:
    IfEffect(std::unique_ptr<ICondition> cond, std::vector<std::unique_ptr<IEffect>> effects);
    ~IfEffect() override; // 析构在 .cpp 定义（unique_ptr<ICondition> 需要完整类型）
    std::string apply(GameContext& ctx) const override;
    int estimateMoneyDelta() const override;
private:
    std::unique_ptr<ICondition> cond_;
    std::vector<std::unique_ptr<IEffect>> effects_;
};

class EffectFactory {
public:
    static std::unique_ptr<IEffect> create(const Json& j);
};
