#include "events/EndingJudge.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "core/ConfigManager.h"
#include "core/GameContext.h"
#include "events/Condition.h"

const EndingDef* EndingJudge::judge(const GameContext& ctx) {
    // 按 priority 排序判定（不修改 cfg 中的顺序）
    std::vector<const EndingDef*> sorted;
    for (const auto& e : ctx.config.endings()) sorted.push_back(&e);
    std::sort(sorted.begin(), sorted.end(),
              [](const EndingDef* a, const EndingDef* b) { return a->priority < b->priority; });

    for (const auto* e : sorted) {
        bool all = true;
        for (const auto& cj : e->conditions) {
            auto cond = ConditionFactory::create(cj);
            bool ok = cond->test(ctx);
#ifdef ENDING_DEBUG
            std::cout << "  [debug] ending=" << e->id << " type=" << cj.value("type", "?")
                      << " ok=" << ok << "\n";
#endif
            if (!ok) {
                all = false;
                break;
            }
        }
        if (all) return e;
    }
    throw std::runtime_error("endings.json 缺少兜底结局（priority=99, always）");
}
