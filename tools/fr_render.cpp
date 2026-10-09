// Offline renderer: impulse or WAV in, 32-bit float stereo WAV out.
//   fr_render --impulse 8 --out ir.wav [--rate 48000] [--quality eco|normal|high]
//             [--size 1] [--t60 3 2 1] [--mod 1 0.7] [--er 0] [--tail 1] [--seed 1] [--block 512]
//   fr_render --in dry.wav --out wet.wav ...   (PCM16/24/32 or float32 WAV in)
#include "reverb.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {
struct Audio { int rate = 48000; std::vector<float> l, r; };

bool readWav(const char* path, Audio& a) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    std::vector<unsigned char> d;
    unsigned char tmp[65536]; size_t k;
    while ((k = std::fread(tmp, 1, sizeof tmp, f)) > 0) d.insert(d.end(), tmp, tmp + k);
    std::fclose(f);
    if (d.size() < 44 || std::memcmp(&d[0], "RIFF", 4) || std::memcmp(&d[8], "WAVE", 4)) return false;
    int fmt = 0, ch = 0, bits = 0;
    size_t p = 12;
    while (p + 8 <= d.size()) {
        uint32_t sz; std::memcpy(&sz, &d[p + 4], 4);
        if (!std::memcmp(&d[p], "fmt ", 4)) {
            uint16_t t, c, b; uint32_t sr;
            std::memcpy(&t, &d[p + 8], 2); std::memcpy(&c, &d[p + 10], 2);
            std::memcpy(&sr, &d[p + 12], 4); std::memcpy(&b, &d[p + 22], 2);
            fmt = t; ch = c; bits = b; a.rate = (int)sr;
        } else if (!std::memcmp(&d[p], "data", 4)) {
            size_t n = std::min<size_t>(sz, d.size() - p - 8) / (bits / 8) / std::max(ch, 1);
            const unsigned char* q = &d[p + 8];
            a.l.resize(n); a.r.resize(n);
            for (size_t i = 0; i < n; ++i) {
                float v[2] = {0, 0};
                for (int c = 0; c < std::min(ch, 2); ++c) {
                    const unsigned char* s = q + (i * ch + c) * (bits / 8);
                    if (fmt == 3 && bits == 32) std::memcpy(&v[c], s, 4);
                    else if (bits == 16) { int16_t x; std::memcpy(&x, s, 2); v[c] = x / 32768.f; }
                    else if (bits == 24) { int32_t x = (s[0] << 8 | s[1] << 16 | (uint32_t)s[2] << 24) >> 8; v[c] = x / 8388608.f; }
                    else if (bits == 32) { int32_t x; std::memcpy(&x, s, 4); v[c] = x / 2147483648.f; }
                }
                a.l[i] = v[0]; a.r[i] = ch > 1 ? v[1] : v[0];
            }
            return true;
        }
        p += 8 + sz + (sz & 1);
    }
    return false;
}

bool writeWav(const char* path, const Audio& a) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    uint32_t n = (uint32_t)a.l.size(), dataBytes = n * 8, riff = 36 + dataBytes, rate = a.rate, br = rate * 8, sixteen = 16;
    uint16_t fmt = 3, ch = 2, align = 8, bits = 32;
    std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f);
    std::fwrite(&sixteen, 4, 1, f); std::fwrite(&fmt, 2, 1, f); std::fwrite(&ch, 2, 1, f);
    std::fwrite(&rate, 4, 1, f); std::fwrite(&br, 4, 1, f); std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f);
    std::fwrite("data", 1, 4, f); std::fwrite(&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i) { std::fwrite(&a.l[i], 4, 1, f); std::fwrite(&a.r[i], 4, 1, f); }
    std::fclose(f);
    return true;
}
}  // namespace

int main(int argc, char** argv) {
    fr::ReverbParams p;
    const char *in = nullptr, *out = nullptr;
    double impulseSec = 0; int rate = 48000, block = 512;
    p.erLevel = 0.f;   // tail only by default, so the analysis sees the FDN alone
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto num = [&](int k) { return std::atof(argv[i + k]); };
        if (a == "--in") in = argv[++i];
        else if (a == "--out") out = argv[++i];
        else if (a == "--impulse") impulseSec = num(1), ++i;
        else if (a == "--rate") rate = (int)num(1), ++i;
        else if (a == "--block") block = (int)num(1), ++i;
        else if (a == "--size") p.size = num(1), ++i;
        else if (a == "--t60") { for (int k = 0; k < 3; ++k) p.t60[k] = num(1 + k); i += 3; }
        else if (a == "--mod") { p.modDepth = num(1); p.modRateHz = num(2); i += 2; }
        else if (a == "--er") p.erLevel = (float)num(1), ++i;
        else if (a == "--tail") p.tailLevel = (float)num(1), ++i;
        else if (a == "--seed") p.seed = (uint64_t)num(1), ++i;
        else if (a == "--predelay") p.preDelayMs = num(1), ++i;
        else if (a == "--no-diffusion") p.diffusion = false;
        else if (a == "--quality") {
            std::string q = argv[++i];
            p.quality = q == "eco" ? fr::Quality::Eco : q == "high" ? fr::Quality::High : fr::Quality::Normal;
        } else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
    }
    if (!out || (!in && impulseSec <= 0)) { std::fprintf(stderr, "need --out and (--in or --impulse)\n"); return 2; }
    Audio src;
    if (in) { if (!readWav(in, src)) { std::fprintf(stderr, "cannot read %s\n", in); return 1; } rate = src.rate; }
    else { src.rate = rate; src.l.assign((size_t)(impulseSec * rate), 0.f); src.r = src.l; src.l[0] = src.r[0] = 1.f; }
    fr::Reverb rv;
    rv.configure(p, rate, block);
    Audio res; res.rate = rate; res.l.resize(src.l.size()); res.r.resize(src.l.size());
    for (size_t off = 0; off < src.l.size(); off += block) {   // host-style blocks
        int n = (int)std::min<size_t>(block, src.l.size() - off);
        rv.process(&src.l[off], &src.r[off], &res.l[off], &res.r[off], n);
    }
    if (!writeWav(out, res)) { std::fprintf(stderr, "cannot write %s\n", out); return 1; }
    return 0;
}
