#pragma once
// ============================================================
// [core/EventBus] 观察者模式事件总线（组长 A 维护，W1 冻结）
// 分工 7.2：交易完成、周结算、事件触发、结局等通过 EventBus 广播，
// UI（E）订阅刷新，不产生反向依赖。类型安全：按事件类型订阅/发布。
// ============================================================
#include <algorithm>
#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class EventBus {
public:
    using HandlerId = std::uint64_t;

    EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // 返回句柄：订阅方析构时必须 unsubscribe，否则会留下悬垂捕获（GameContext 已有教训）
    template <typename E>
    HandlerId subscribe(std::function<void(const E&)> handler) {
        const HandlerId id = nextId_++;
        auto eraser = [h = std::move(handler)](const void* p) {
            h(*static_cast<const E*>(p));
        };
        subs_[std::type_index(typeid(E))].push_back(Entry{id, std::move(eraser)});
        return id;
    }

    // 退订：按句柄扫一遍所有事件类型（订阅数很少，线性足够）
    void unsubscribe(HandlerId id) {
        for (auto& kv : subs_) {
            auto& handlers = kv.second;
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                               [id](const Entry& e) { return e.id == id; }),
                handlers.end());
        }
    }

    template <typename E>
    void publish(const E& event) {
        auto it = subs_.find(std::type_index(typeid(E)));
        if (it == subs_.end()) return;
        // 拷贝一份，允许处理器内部再订阅/取消订阅而不破坏迭代
        std::vector<Entry> snapshot = it->second;
        for (const auto& e : snapshot) e.fn(&event);
    }

private:
    using Handler = std::function<void(const void*)>;
    struct Entry {
        HandlerId id = 0;
        Handler fn;
    };
    std::unordered_map<std::type_index, std::vector<Entry>> subs_;
    HandlerId nextId_ = 1;
};
