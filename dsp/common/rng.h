#pragma once
#include <cstdint>
#include <cmath>
// Small deterministic RNG. Identical output on every platform (no std distributions).
namespace fr {
class Rng {
public:
    explicit Rng(uint64_t seed = 1) : s_(seed * 0x9E3779B97F4A7C15ull + 0xD1B54A32D192ED03ull) {}
    uint64_t next() {  // splitmix64
        uint64_t z = (s_ += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    double uniform() { return ((next() >> 11) + 0.5) * (1.0 / 9007199254740992.0); }  // (0,1)
    double gauss() {  // Box-Muller
        double u1 = uniform(), u2 = uniform();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(6.283185307179586 * u2);
    }
private:
    uint64_t s_;
};
}
