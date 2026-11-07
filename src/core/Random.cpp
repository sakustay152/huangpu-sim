#include "core/Random.h"

#include "util/JsonUtil.h"

std::uint64_t Random::next() {
    std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

int Random::range(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + static_cast<int>(next() % static_cast<std::uint64_t>(hi - lo + 1));
}

double Random::real() {
    return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
}

bool Random::chance(double p) {
    if (p >= 1.0) return true;
    if (p <= 0.0) return false;
    return real() < p;
}

double Random::uniform(double lo, double hi) {
    return lo + real() * (hi - lo);
}

Json Random::toJson() const {
    return Json{{"state", state_}};
}

void Random::fromJson(const Json& j) {
    state_ = ju::getOr(j, "state", state_);
}
