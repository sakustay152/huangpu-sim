#include "events/EventFactory.h"

#include <set>
#include <stdexcept>

#include "core/ConfigManager.h"
#include "events/Condition.h"
#include "events/Effect.h"
#include "events/GameEvent.h"
#include "util/JsonUtil.h"

std::unique_ptr<GameEvent> EventFactory::create(const Json& e, const ConfigManager& cfg) {
    std::string type = ju::getOr<std::string>(e, "type", "random");
    std::string id = ju::getOr<std::string>(e, "id", "");
    if (id.empty()) throw std::runtime_error("events.json 存在缺少 id 的事件");

    std::unique_ptr<GameEvent> ev;
    if (type == "random") {
        auto r = std::make_unique<RandomEvent>();
        r->weightValue = ju::getOr(e, "weight", 10);
        r->cooldownWeeks = ju::getOr(e, "cooldown", 2);
        ev = std::move(r);
    } else if (type == "fixed") {
        auto f = std::make_unique<FixedEvent>();
        f->week = ju::getOr(e, "week", 1);
        ev = std::move(f);
    } else if (type == "npc") {
        auto n = std::make_unique<NpcEvent>();
        n->npcId = ju::getOr<std::string>(e, "npc", "");
        n->minAffinity = ju::getOr(e, "minAffinity", 0);
        if (!cfg.npc(n->npcId)) throw std::runtime_error("事件 " + id + " 引用了不存在的 NPC: " + n->npcId);
        ev = std::move(n);
    } else {
        throw std::runtime_error("事件 " + id + " 的类型未知: " + type);
    }

    ev->id = id;
    ev->name = ju::getOr<std::string>(e, "name", id);
    ev->text = ju::getOr<std::string>(e, "text", "");
    ev->once = ju::getOr(e, "once", false);

    if (e.contains("conditions") && e["conditions"].is_array())
        for (const auto& c : e["conditions"])
            ev->conditions.push_back(ConditionFactory::create(c));

    if (e.contains("choices") && e["choices"].is_array()) {
        for (const auto& c : e["choices"]) {
            GameEvent::Choice choice;
            choice.text = ju::getOr<std::string>(c, "text", "");
            if (c.contains("effects") && c["effects"].is_array())
                for (const auto& eff : c["effects"])
                    choice.effects.push_back(EffectFactory::create(eff));
            ev->choices.push_back(std::move(choice));
        }
    }
    if (ev->choices.empty()) throw std::runtime_error("事件 " + id + " 没有任何选项");
    return ev;
}

std::vector<std::unique_ptr<GameEvent>> EventFactory::buildAll(const ConfigManager& cfg) {
    std::vector<std::unique_ptr<GameEvent>> out;
    std::set<std::string> ids;
    const Json& raw = cfg.rawEvents();
    for (const auto& e : raw) {
        auto ev = create(e, cfg);
        if (!ids.insert(ev->id).second)
            throw std::runtime_error("events.json 事件 id 重复: " + ev->id);
        out.push_back(std::move(ev));
    }
    return out;
}
