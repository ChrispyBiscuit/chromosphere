// CPU benchmark: stereo noise through the reverb in host-sized blocks.
// Reports average load as % of one core at real time and the worst block time as
// % of the block duration (PRD Section 8.1).  Build with -DFR_NATIVE=ON for local numbers.
#include "reverb.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

struct Case { const char* name; fr::Quality q; double fs; int block; double budgetPct; };

int main() {
    const Case cases[] = {
        {"Normal 48k/128", fr::Quality::Normal, 48000, 128, 2.0},
        {"High   48k/128", fr::Quality::High,   48000, 128, 5.0},
        {"Eco    48k/64 ", fr::Quality::Eco,    48000, 64,  1.0},
        {"Normal 96k/128", fr::Quality::Normal, 96000, 128, 2.0 * 2.2},
    };
    std::printf("%-16s %9s %9s %12s %12s\n", "case", "avg %core", "budget", "p99.9 block%", "worst block%");
    for (const Case& c : cases) {
        fr::ReverbParams p; p.quality = c.q; p.erLevel = 0.5f;
        fr::Reverb rv; rv.configure(p, c.fs, c.block);
        const int seconds = 2, repeats = 5, blocks = (int)(c.fs * seconds) / c.block;
        std::vector<float> in(c.block), out(c.block), in2(c.block), out2(c.block);
        unsigned s = 12345;
        auto fill = [&](std::vector<float>& v) { for (float& x : v) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * 0.3f; } };
        for (int b = 0; b < 200; ++b) { fill(in); fill(in2); rv.process(in.data(), in2.data(), out.data(), out2.data(), c.block); }  // warm up
        double total = 1e30, worst = 0;                    // best of `repeats` runs: least scheduler noise
        std::vector<double> times; times.reserve(blocks * repeats);
        for (int rep = 0; rep < repeats; ++rep) {
            double t = 0;
            for (int b = 0; b < blocks; ++b) {
                fill(in); fill(in2);
                auto t0 = std::chrono::steady_clock::now();
                rv.process(in.data(), in2.data(), out.data(), out2.data(), c.block);
                double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                t += dt; times.push_back(dt);
            }
            total = std::min(total, t);
        }
        worst = *std::max_element(times.begin(), times.end());
        std::sort(times.begin(), times.end());
        double blockSec = c.block / c.fs, p999 = times[(size_t)(times.size() * 0.999)];
        std::printf("%-16s %8.2f%% %8.1f%% %11.1f%% %11.1f%%\n", c.name, 100.0 * total / seconds, c.budgetPct, 100.0 * p999 / blockSec, 100.0 * worst / blockSec);
    }
}
