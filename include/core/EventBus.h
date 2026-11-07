#pragma once
// ============================================================
// [core/EventBus] 观察者模式事件总线（组长 A 维护，W1 冻结）
// 分工 7.2：交易完成、周结算、事件触发、结局等通过 EventBus 广播，
// UI（E）订阅刷新，不产生反向依赖。类型安全：按事件类型订阅/发布。
// ============================================================
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

class EventBus {
public:
    EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    template <typename E>
    void subscribe(std::function<void(const E&)> handler) {
        auto eraser = [h = std::move(handler)](const void* p) {
            h(*static_cast<const E*>(p));
        };
        subs_[std::type_index(typeid(E))].push_back(std::move(eraser));
    }

    template <typename E>
    void publish(const E& event) {
        auto it = subs_.find(std::type_index(typeid(E)));
        if (it == subs_.end()) return;
        // 拷贝一份，允许处理器内部再订阅/取消订阅而不破坏迭代
        std::vector<Handler> snapshot = it->second;
        for (const auto& h : snapshot) h(&event);
    }

private:
    using Handler = std::function<void(const void*)>;
    std::unordered_map<std::type_index, std::vector<Handler>> subs_;
};
