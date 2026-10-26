// One second of audio through the reverb; used under valgrind by tools/perf_check.py.
#include "reverb.h"
#include <cstring>
#include <vector>
int main(int argc, char** argv) {
    cs::ReverbParams p; p.erLevel = 0.5f;
    if (argc > 1 && !std::strcmp(argv[1], "eco")) p.quality = cs::Quality::Eco;
    if (argc > 1 && !std::strcmp(argv[1], "high")) p.quality = cs::Quality::High;
    cs::Reverb rv; rv.configure(p, 48000, 128);
    std::vector<float> a(128), l(128), r(128);
    unsigned s = 1;
    for (int b = 0; b < 48000 / 128; ++b) {
        for (float& x : a) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * 0.3f; }
        rv.process(a.data(), a.data(), l.data(), r.data(), 128);
    }
    return 0;
}
