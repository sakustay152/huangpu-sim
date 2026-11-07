#pragma once
// ============================================================
// [story/StoryFlags] 剧情 flag（组员 D 主责；A 集成最小版）
// 一学期里"发生过什么"的集合：社团、借钱、内部票……
// 结局判定与事件触发条件都读它。
// ============================================================
#include <set>
#include <string>

#include "core/ISaveable.h"

class StoryFlags : public ISaveable {
public:
    bool has(const std::string& flag) const { return flags_.count(flag) > 0; }
    void set(const std::string& flag) { flags_.insert(flag); }
    void erase(const std::string& flag) { flags_.erase(flag); }
    const std::set<std::string>& all() const { return flags_; }

    Json toJson() const override;
    void fromJson(const Json& j) override;

private:
    std::set<std::string> flags_;
};
