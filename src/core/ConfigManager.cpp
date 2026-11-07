#include "core/ConfigManager.h"

#include <stdexcept>

#include "util/JsonUtil.h"

ConfigManager::ConfigManager(const std::string& dataDir) : dataDir_(dataDir) {
    loadBalance();
    loadGoods();
    loadLocations();
    loadNpcs();
    loadEndings();
    loadEvents();

    // ---- 交叉校验：引用完整性 ----
    for (const auto& g : goods_) {
        if (g.basePrice <= 0) fail("商品 " + g.id + " 基准价必须为正");
        if (g.slots <= 0) fail("商品 " + g.id + " 占格必须为正");
        if (g.limitPerDay <= 0) fail("商品 " + g.id + " 限购必须为正");
        if (g.soldAt.empty()) fail("商品 " + g.id + " 没有任何有售地点");
        for (const auto& loc : g.soldAt)
            if (!location(loc)) fail("商品 " + g.id + " 引用了不存在的地点 " + loc);
        for (const auto& [loc, f] : g.locFactor)
            if (!location(loc)) fail("商品 " + g.id + " 的价格系数引用了不存在的地点 " + loc);
    }
    for (const auto& n : npcs_)
        if (!location(n.home)) fail("NPC " + n.id + " 的驻地不存在: " + n.home);
    for (const auto& [loc, range] : balance_.jobs)
        if (!location(loc)) fail("balance.json jobs 引用了不存在的地点 " + loc);
    if (balance_.weeks <= 0 || balance_.apPerWeek <= 0) fail("周数/行动点必须为正");
}

std::string ConfigManager::pathOf(const char* name) const {
    return dataDir_ + "/" + name;
}

[[noreturn]] void ConfigManager::fail(const std::string& what) const {
    throw std::runtime_error("[数据校验失败] " + what);
}

void ConfigManager::loadBalance() {
    Json j = ju::loadFile(pathOf("balance.json"));
    balance_.startMoney   = ju::getOr(j, "startMoney", 3000);
    balance_.weeklyExpense = ju::getOr(j, "weeklyExpense", 250);
    balance_.targetMoney  = ju::getOr(j, "targetMoney", 12000);
    balance_.weeks        = ju::getOr(j, "weeks", 12);
    balance_.apPerWeek    = ju::getOr(j, "apPerWeek", 6);
    balance_.maxSlots     = ju::getOr(j, "maxSlots", 30);
    balance_.studyStart   = ju::getOr(j, "studyStart", 40);
    balance_.studyGain    = ju::getOr(j, "studyGain", 6);
    balance_.studyDecay   = ju::getOr(j, "studyDecay", 4);
    balance_.passScore    = ju::getOr(j, "passScore", 60);
    balance_.warnScore    = ju::getOr(j, "warnScore", 50);
    balance_.scholarshipScore = ju::getOr(j, "scholarshipScore", 85);
    balance_.scholarship  = ju::getOr(j, "scholarship", 1000);
    balance_.retakeFee    = ju::getOr(j, "retakeFee", 2000);
    balance_.socialGain   = ju::getOr(j, "socialGain", 15);
    balance_.restAffinity = ju::getOr(j, "restAffinity", 3);
    balance_.eventChance  = ju::getOr(j, "eventChance", 0.22);
    balance_.rumorAccuracy = ju::getOr(j, "rumorAccuracy", 0.8);
    balance_.trendAmp     = ju::getOr(j, "trendAmp", 0.22);
    balance_.noiseAmp     = ju::getOr(j, "noiseAmp", 0.06);
    balance_.seasonAmp    = ju::getOr(j, "seasonAmp", 0.35);
    balance_.kaoyanDiscount   = ju::getOr(j, "kaoyanDiscount", 0.9);
    balance_.innerTicketBonus = ju::getOr(j, "innerTicketBonus", 1.25);
    balance_.seniorWorkBonus  = ju::getOr(j, "seniorWorkBonus", 1.2);

    if (j.contains("jobs") && j["jobs"].is_object()) {
        for (auto it = j["jobs"].begin(); it != j["jobs"].end(); ++it) {
            if (it.value().is_array() && it.value().size() >= 2)
                balance_.jobs[it.key()] = {it.value()[0].get<int>(), it.value()[1].get<int>()};
        }
    }
}

void ConfigManager::loadGoods() {
    Json j = ju::loadFile(pathOf("goods.json"));
    if (!j.is_array()) fail("goods.json 顶层必须是数组");
    for (const auto& e : j) {
        GoodsDef g;
        g.id         = ju::getOr<std::string>(e, "id", "");
        g.name       = ju::getOr<std::string>(e, "name", g.id);
        g.category   = ju::getOr<std::string>(e, "category", "");
        g.basePrice  = ju::getOr(e, "basePrice", 1);
        g.spread     = ju::getOr(e, "spread", 0.08);
        g.slots      = ju::getOr(e, "slots", 1);
        g.limitPerDay = ju::getOr(e, "limitPerDay", 15);
        g.desc       = ju::getOr<std::string>(e, "desc", "");
        if (g.id.empty()) fail("goods.json 存在缺少 id 的商品");
        if (e.contains("soldAt") && e["soldAt"].is_array())
            for (const auto& s : e["soldAt"]) g.soldAt.push_back(s.get<std::string>());
        if (e.contains("locFactor") && e["locFactor"].is_object())
            for (auto it = e["locFactor"].begin(); it != e["locFactor"].end(); ++it)
                g.locFactor[it.key()] = it.value().get<double>();
        if (e.contains("peakWeeks") && e["peakWeeks"].is_array())
            for (const auto& w : e["peakWeeks"]) g.peakWeeks.push_back(w.get<int>());
        goods_.push_back(std::move(g));
    }
    if (goods_.empty()) fail("goods.json 没有任何商品");
}

void ConfigManager::loadLocations() {
    Json j = ju::loadFile(pathOf("locations.json"));
    if (!j.is_array()) fail("locations.json 顶层必须是数组");
    for (const auto& e : j) {
        LocationDef l;
        l.id    = ju::getOr<std::string>(e, "id", "");
        l.name  = ju::getOr<std::string>(e, "name", l.id);
        l.desc  = ju::getOr<std::string>(e, "desc", "");
        l.fare  = ju::getOr(e, "fare", 2);
        l.npcId = ju::getOr<std::string>(e, "npcId", "");
        if (l.id.empty()) fail("locations.json 存在缺少 id 的地点");
        locations_.push_back(std::move(l));
    }
    if (locations_.empty()) fail("locations.json 没有任何地点");
}

void ConfigManager::loadNpcs() {
    Json j = ju::loadFile(pathOf("npcs.json"));
    if (!j.is_array()) return;
    for (const auto& e : j) {
        NpcDef n;
        n.id    = ju::getOr<std::string>(e, "id", "");
        n.name  = ju::getOr<std::string>(e, "name", n.id);
        n.home  = ju::getOr<std::string>(e, "home", "");
        n.intro = ju::getOr<std::string>(e, "intro", "");
        if (!n.id.empty()) npcs_.push_back(std::move(n));
    }
}

void ConfigManager::loadEndings() {
    Json j = ju::loadFile(pathOf("endings.json"));
    if (!j.is_array()) fail("endings.json 顶层必须是数组");
    for (const auto& e : j) {
        EndingDef d;
        d.id       = ju::getOr<std::string>(e, "id", "");
        d.title    = ju::getOr<std::string>(e, "title", d.id);
        d.text     = ju::getOr<std::string>(e, "text", "");
        d.priority = ju::getOr(e, "priority", 99);
        if (e.contains("conditions") && e["conditions"].is_array())
            for (const auto& c : e["conditions"]) d.conditions.push_back(c);
        if (!d.id.empty()) endings_.push_back(std::move(d));
    }
    if (endings_.empty()) fail("endings.json 至少需要一个结局");
}

void ConfigManager::loadEvents() {
    rawEvents_ = ju::loadFile(pathOf("events.json"));
    if (!rawEvents_.is_array()) fail("events.json 顶层必须是数组");
}

const GoodsDef* ConfigManager::goods(const std::string& id) const {
    for (const auto& g : goods_)
        if (g.id == id) return &g;
    return nullptr;
}

const LocationDef* ConfigManager::location(const std::string& id) const {
    for (const auto& l : locations_)
        if (l.id == id) return &l;
    return nullptr;
}

const NpcDef* ConfigManager::npc(const std::string& id) const {
    for (const auto& n : npcs_)
        if (n.id == id) return &n;
    return nullptr;
}
