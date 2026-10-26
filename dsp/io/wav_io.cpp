#include "io/wav_io.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace cs {
bool readWav(const std::string& path, WavData& a) {
    FILE* f = std::fopen(path.c_str(), "rb");
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
            if (fmt == 0xFFFE && sz >= 26) { uint16_t sub; std::memcpy(&sub, &d[p + 8 + 24], 2); fmt = sub; }   // WAVE_FORMAT_EXTENSIBLE
        } else if (!std::memcmp(&d[p], "data", 4)) {
            if (ch < 1 || bits < 8) return false;
            size_t n = std::min<size_t>(sz, d.size() - p - 8) / (bits / 8) / ch;
            const unsigned char* q = &d[p + 8];
            a.ch.assign(ch, std::vector<float>(n));
            for (size_t i = 0; i < n; ++i)
                for (int c = 0; c < ch; ++c) {
                    const unsigned char* s = q + (i * ch + c) * (bits / 8);
                    float v = 0;
                    if (fmt == 3 && bits == 32) std::memcpy(&v, s, 4);
                    else if (bits == 16) { int16_t x; std::memcpy(&x, s, 2); v = x / 32768.f; }
                    else if (bits == 24) { int32_t x = (int32_t)((uint32_t)s[0] << 8 | (uint32_t)s[1] << 16 | (uint32_t)s[2] << 24) >> 8; v = x / 8388608.f; }
                    else if (bits == 32) { int32_t x; std::memcpy(&x, s, 4); v = x / 2147483648.f; }
                    a.ch[c][i] = v;
                }
            return true;
        }
        p += 8 + sz + (sz & 1);
    }
    return false;
}

bool writeWav(const std::string& path, const WavData& a) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const uint16_t chans = (uint16_t)a.ch.size();
    const uint32_t n = a.ch.empty() ? 0 : (uint32_t)a.ch[0].size(), dataBytes = n * chans * 4, riff = 36 + dataBytes,
                   rate = (uint32_t)a.rate, br = rate * chans * 4, sixteen = 16;
    const uint16_t fmt = 3, align = (uint16_t)(chans * 4), bits = 32;
    std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f);
    std::fwrite(&sixteen, 4, 1, f); std::fwrite(&fmt, 2, 1, f); std::fwrite(&chans, 2, 1, f);
    std::fwrite(&rate, 4, 1, f); std::fwrite(&br, 4, 1, f); std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f);
    std::fwrite("data", 1, 4, f); std::fwrite(&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i) for (uint16_t c = 0; c < chans; ++c) std::fwrite(&a.ch[c][i], 4, 1, f);
    std::fclose(f);
    return true;
}
}
