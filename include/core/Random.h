#pragma once
// ============================================================
// [core/Random] 可复现随机数（组长 A 维护）
// 分工 8.3：所有随机走统一 Random + 种子，bug 可复现；
// 状态只有一个 uint64，可随存档序列化，读档后随机序列完全一致。
// 算法：splitmix64（质量足够、可序列化、无外部依赖）
// ============================================================
#include <cstdint>
#include <vector>

#include "util/Json.h"

class Random {
public:
    explicit Random(std::uint64_t seed = 20260901ull) : state_(seed ? seed : 1) {}

    void reseed(std::uint64_t seed) { state_ = seed ? seed : 1; }

    std::uint64_t next();                       // [0, 2^64)
    int range(int lo, int hi);                  // [lo, hi] 闭区间整数
    double real();                              // [0, 1)
    bool chance(double p);                      // 以概率 p 为真
    double uniform(double lo, double hi);       // [lo, hi) 浮点

    template <typename T>
    const T& pick(const std::vector<T>& v) { return v[next() % v.size()]; }

    // ISaveable 语义（随机状态随存档走）
    Json toJson() const;
    void fromJson(const Json& j);

private:
    std::uint64_t state_;
};
