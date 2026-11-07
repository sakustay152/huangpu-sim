#include "player/Inventory.h"

#include "util/JsonUtil.h"

int Inventory::usedSlots() const {
    int s = 0;
    for (const auto& it : items_) s += it.qty * it.slotsPer;
    return s;
}

const Inventory::Item* Inventory::find(const std::string& goodsId) const {
    for (const auto& it : items_)
        if (it.goodsId == goodsId) return &it;
    return nullptr;
}

Inventory::Item* Inventory::findMutable(const std::string& goodsId) {
    for (auto& it : items_)
        if (it.goodsId == goodsId) return &it;
    return nullptr;
}

void Inventory::add(const std::string& goodsId, int qty, double unitCost, int slotsPer) {
    if (qty <= 0) return;
    Item* it = findMutable(goodsId);
    if (it) {
        double totalCost = it->avgCost * it->qty + unitCost * qty;
        it->qty += qty;
        it->avgCost = totalCost / it->qty;
    } else {
        items_.push_back({goodsId, qty, slotsPer, unitCost});
    }
}

bool Inventory::remove(const std::string& goodsId, int qty) {
    Item* it = findMutable(goodsId);
    if (!it || it->qty < qty) return false;
    it->qty -= qty;
    if (it->qty == 0) {
        // 保持顺序，移除空堆叠
        for (std::size_t i = 0; i < items_.size(); ++i) {
            if (items_[i].goodsId == goodsId) {
                items_.erase(items_.begin() + static_cast<long>(i));
                break;
            }
        }
    }
    return true;
}

Json Inventory::toJson() const {
    Json arr = Json::array();
    for (const auto& it : items_) {
        arr.push_back(Json{
            {"goodsId", it.goodsId},
            {"qty", it.qty},
            {"slotsPer", it.slotsPer},
            {"avgCost", it.avgCost},
        });
    }
    return Json{{"capacity", capacity_}, {"items", arr}};
}

void Inventory::fromJson(const Json& j) {
    capacity_ = ju::getOr(j, "capacity", capacity_);
    items_.clear();
    if (!j.contains("items") || !j["items"].is_array()) return;
    for (const auto& e : j["items"]) {
        Item it;
        it.goodsId  = ju::getOr<std::string>(e, "goodsId", "");
        it.qty      = ju::getOr(e, "qty", 0);
        it.slotsPer = ju::getOr(e, "slotsPer", 1);
        it.avgCost  = ju::getOr(e, "avgCost", 0.0);
        if (!it.goodsId.empty() && it.qty > 0) items_.push_back(std::move(it));
    }
}
