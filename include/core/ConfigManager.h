#pragma once
// ============================================================
// [core/ConfigManager] 配置管理器（组长 A 维护）
// 一次性加载 data/*.json 并做交叉校验（分工 7.2）：
//   - 商品的有售地点/价格系数引用的地点必须存在
//   - NPC 的驻地必须存在
//   - 数值必须合法（价格为正、区间有序）
// 配置错误在启动时暴露（fail fast），运行期不再怀疑数据。
// ============================================================
#include <string>
#include <vector>

#include "core/ConfigDefs.h"

class ConfigManager {
public:
    explicit ConfigManager(const std::string& dataDir); // 解析失败抛 std::runtime_error

    const BalanceDef& balance() const { return balance_; }

    const std::vector<GoodsDef>& goods() const { return goods_; }
    const GoodsDef* goods(const std::string& id) const;

    const std::vector<LocationDef>& locations() const { return locations_; }
    const LocationDef* location(const std::string& id) const;

    const std::vector<NpcDef>& npcs() const { return npcs_; }
    const NpcDef* npc(const std::string& id) const;

    const std::vector<EndingDef>& endings() const { return endings_; }

    // events.json 原文（EventScheduler 启动时用 EventFactory 解析）
    const Json& rawEvents() const { return rawEvents_; }

    // social.json：NPC 社交选项池（v2 社交三选一）
    const SocialPoolDef* socialPool(const std::string& npcId) const;

    const std::string& dataDir() const { return dataDir_; }

private:
    std::string dataDir_;
    std::string pathOf(const char* name) const;

    BalanceDef balance_;
    std::vector<GoodsDef> goods_;
    std::vector<LocationDef> locations_;
    std::vector<NpcDef> npcs_;
    std::vector<EndingDef> endings_;
    Json rawEvents_;
    std::map<std::string, SocialPoolDef> socialPools_; // npcId → 选项池

    void loadBalance();
    void loadGoods();
    void loadLocations();
    void loadNpcs();
    void loadEndings();
    void loadEvents();
    void loadSocialPools();

    [[noreturn]] void fail(const std::string& what) const;
};
