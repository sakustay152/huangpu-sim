#pragma once
// ============================================================
// [events/EventFactory] 事件工厂（D 主责的机制；A 集成最小版）
// Factory 模式：events.json → GameEvent 继承树；
// 内容（文案/数值/条件/效果）全在 JSON，代码不硬编码。
// ============================================================
#include <memory>
#include <vector>

#include "util/Json.h"

class ConfigManager;
class GameEvent;

class EventFactory {
public:
    // 解析全部事件；任何引用错误（未知条件/效果/商品/NPC）直接抛异常
    static std::vector<std::unique_ptr<GameEvent>> buildAll(const ConfigManager& cfg);

private:
    static std::unique_ptr<GameEvent> create(const Json& e, const ConfigManager& cfg);
};
