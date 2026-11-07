#pragma once
// ============================================================
// [player/Inventory] 背包（组员 C 主责；A 集成最小版）
// 封装：格子容量（30 格，大件占 4 格）、堆叠、加权平均成本
// （卖出时算盈亏）。容量校验只在本类入口做，保证"不超容"不变量。
// ============================================================
#include <string>
#include <vector>

#include "core/ISaveable.h"

class Inventory : public ISaveable {
public:
    struct Item {
        std::string goodsId;
        int qty = 0;
        int slotsPer = 1;  // 每件占格（大件 4）
        double avgCost = 0.0; // 加权平均进货价
    };

    explicit Inventory(int capacity) : capacity_(capacity) {}

    int capacity() const { return capacity_; }
    int usedSlots() const;
    int freeSlots() const { return capacity_ - usedSlots(); }
    bool canFit(int qty, int slotsPer) const { return qty * slotsPer <= freeSlots(); }

    const Item* find(const std::string& goodsId) const;
    Item* findMutable(const std::string& goodsId);
    const std::vector<Item>& items() const { return items_; }
    bool empty() const { return items_.empty(); }

    // 入包（调用方保证已 canFit）；加权平均成本在此维护
    void add(const std::string& goodsId, int qty, double unitCost, int slotsPer);

    // 出包：数量不足返回 false
    bool remove(const std::string& goodsId, int qty);

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    int capacity_;
    std::vector<Item> items_;
};
