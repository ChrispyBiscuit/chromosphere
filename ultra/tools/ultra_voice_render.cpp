// Renders one drum voice to a mono 32-bit float WAV at 48 kHz, for listening and analysis (tools/voice_report.py).
// Usage: ultra_voice_render out.wav model tune decay tone snap color sweep [velocity] [seconds] [drive]
//        model is the index into kModelNames (0..12).
#include "ultra/voice.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace ultra;

static void writeWav(const char* path, const std::vector<float>& x, int fs) {
    FILE* f = std::fopen(path, "wb");
    if (!f) { std::perror(path); std::exit(1); }
    const uint32_t data = (uint32_t)(x.size() * 4), riff = 36 + data, fmtLen = 16, rate = (uint32_t)fs, bps = rate * 4;
    const uint16_t fmt = 3, ch = 1, align = 4, bits = 32;
    std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f);
    std::fwrite(&fmtLen, 4, 1, f); std::fwrite(&fmt, 2, 1, f); std::fwrite(&ch, 2, 1, f); std::fwrite(&rate, 4, 1, f);
    std::fwrite(&bps, 4, 1, f); std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f);
    std::fwrite("data", 1, 4, f); std::fwrite(&data, 4, 1, f); std::fwrite(x.data(), 4, x.size(), f);
    std::fclose(f);
}

int main(int argc, char** argv) {
    if (argc < 9) {
        std::fprintf(stderr, "usage: %s out.wav model tune decay tone snap color sweep [velocity] [seconds] [drive]\n", argv[0]);
        return 2;
    }
    const int fs = 48000;
    VoiceParams v;
    v.model = (Model)std::atoi(argv[2]);
    v.tune = (float)std::atof(argv[3]); v.decay = (float)std::atof(argv[4]); v.tone = (float)std::atof(argv[5]);
    v.snap = (float)std::atof(argv[6]); v.color = (float)std::atof(argv[7]); v.sweep = (float)std::atof(argv[8]);
    const float vel = argc > 9 ? (float)std::atof(argv[9]) : 1.f;
    const float secs = argc > 10 ? (float)std::atof(argv[10]) : 1.5f;
    v.drive = argc > 11 ? (float)std::atof(argv[11]) : 0.f;
    DrumVoice d; d.prepare(fs, 0x1234u);
    d.setFilter(FilterType::Off, 20000.f, 0.1f);
    d.trigger(v, 1.f, vel);
    std::vector<float> x((size_t)(secs * fs), 0.f);
    d.render(x.data(), (int)x.size());
    writeWav(argv[1], x, fs);
    return 0;
}
