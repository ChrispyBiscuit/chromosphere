// wave_analyze: runs the WAVE engine over a recording and prints the notes it plays; optionally writes them as a
// standard MIDI file. For checking WAVE on real voices without a host.
//
//   wave_analyze voice.wav [out.mid] [--range 0..3] [--gate dB] [--key 0..11] [--scale 0..12] [--chords]
//                [--chord 0..7] [--top] [--bass 0..2] [--strum ms] [--stability ms] [--octave n] [--hold] [--trace]
#include "io/wav_io.h"
#include "wave/engine.h"
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {
const char* noteName(int m) {
    static char buf[16];
    std::snprintf(buf, sizeof buf, "%s%d", wave::kKeyNames[m % 12], m / 12 - 1);
    return buf;
}

void put32(std::vector<uint8_t>& v, uint32_t x) { for (int s = 24; s >= 0; s -= 8) v.push_back((uint8_t)(x >> s)); }
void putVar(std::vector<uint8_t>& v, uint32_t x) {
    uint8_t tmp[5]; int n = 0;
    tmp[n++] = x & 0x7f;
    while (x >>= 7) tmp[n++] = (uint8_t)(0x80 | (x & 0x7f));
    while (n) v.push_back(tmp[--n]);
}

// Format 0, 480 ticks per quarter at 120 bpm (960 ticks per second).
bool writeMidi(const char* path, const std::vector<std::pair<double, wave::Event>>& ev) {
    std::vector<uint8_t> trk;
    uint32_t last = 0;
    for (const auto& [t, e] : ev) {
        const uint32_t tick = (uint32_t)(t * 960.0 + 0.5);
        uint8_t msg[3]; int len = 3;
        switch (e.type) {
            case wave::Event::NoteOn: msg[0] = 0x90; msg[1] = (uint8_t)e.note; msg[2] = (uint8_t)std::max(1, (int)(e.value * 127.f + 0.5f)); break;
            case wave::Event::NoteOff: msg[0] = 0x80; msg[1] = (uint8_t)e.note; msg[2] = 0; break;
            case wave::Event::Aftertouch: msg[0] = 0xD0; msg[1] = (uint8_t)(e.value * 127.f + 0.5f); len = 2; break;
            case wave::Event::ModWheel: msg[0] = 0xB0; msg[1] = 1; msg[2] = (uint8_t)(e.value * 127.f + 0.5f); break;
            default: continue;                               // per-note tuning has no MIDI 1.0 form
        }
        putVar(trk, tick - last); last = tick;
        trk.insert(trk.end(), msg, msg + len);
    }
    putVar(trk, 0); trk.push_back(0xFF); trk.push_back(0x2F); trk.push_back(0);
    std::vector<uint8_t> f = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0x01, 0xE0, 'M', 'T', 'r', 'k'};
    put32(f, (uint32_t)trk.size());
    f.insert(f.end(), trk.begin(), trk.end());
    FILE* fp = std::fopen(path, "wb");
    if (!fp) return false;
    const bool ok = std::fwrite(f.data(), 1, f.size(), fp) == f.size();
    std::fclose(fp);
    return ok;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: wave_analyze voice.wav [out.mid] [options]; see the source for options\n"); return 2; }
    wave::Params p;
    const char* midiPath = nullptr;
    bool trace = false;
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&] { return i + 1 < argc ? std::atof(argv[++i]) : 0.0; };
        if (a == "--range") p.range = (int)next();
        else if (a == "--gate") p.gateDb = (float)next();
        else if (a == "--key") p.key = (int)next();
        else if (a == "--scale") p.scale = (wave::Scale)(int)next();
        else if (a == "--chords") p.mode = wave::Mode::Chords;
        else if (a == "--chord") p.chord = (wave::ChordType)(int)next();
        else if (a == "--top") p.role = wave::Role::Top;
        else if (a == "--bass") p.bass = (int)next();
        else if (a == "--strum") p.strumMs = (float)next();
        else if (a == "--stability") p.stabilityMs = (float)next();
        else if (a == "--octave") p.octave = (int)next();
        else if (a == "--hold") p.hold = true;
        else if (a == "--trace") trace = true;
        else if (a.size() > 4 && a.compare(a.size() - 4, 4, ".mid") == 0) midiPath = argv[i];
        else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
    }
    fr::WavData w;
    if (!fr::readWav(argv[1], w) || w.ch.empty()) { std::fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }
    std::vector<float> mono(w.ch[0].size(), 0.f);
    for (const auto& c : w.ch) for (size_t i = 0; i < mono.size(); ++i) mono[i] += c[i] / (float)w.ch.size();

    wave::Engine e;
    e.setParams(p);
    e.prepare(w.rate);
    e.listen(true);
    std::vector<wave::Event> buf(4096);
    std::vector<std::pair<double, wave::Event>> all;
    int lastTrace = 0;
    for (size_t off = 0; off < mono.size(); off += 512) {
        const int n = (int)std::min<size_t>(512, mono.size() - off);
        const int k = e.process(mono.data() + off, n, buf.data(), (int)buf.size());
        for (int i = 0; i < k; ++i) all.push_back({(off + (size_t)buf[(size_t)i].offset) / (double)w.rate, buf[(size_t)i]});
        if (trace)
            for (int pos = e.tracePos.load(); lastTrace != pos; lastTrace = (lastTrace + 1) % wave::Engine::kTrace)
                std::printf("  trace  pitch %6.2f  level %6.1f dB  note %d\n", e.tracePitch[(size_t)lastTrace].load(),
                            e.traceLevel[(size_t)lastTrace].load(), e.traceNote[(size_t)lastTrace].load());
    }
    const int k = e.releaseAll(buf.data(), (int)buf.size(), 0);
    for (int i = 0; i < k; ++i) all.push_back({mono.size() / (double)w.rate, buf[(size_t)i]});

    int count = 0;
    for (const auto& [t, ev] : all) {
        if (ev.type == wave::Event::NoteOn) { std::printf("%8.3f s  on   %-4s (%3d)  vel %3d\n", t, noteName(ev.note), ev.note, (int)(ev.value * 127.f + 0.5f)); ++count; }
        else if (ev.type == wave::Event::NoteOff) std::printf("%8.3f s  off  %-4s (%3d)\n", t, noteName(ev.note), ev.note);
    }
    const auto key = e.guessKey();
    std::printf("%d notes; analysis at %.0f Hz, hop %.2f ms; sung key looks like %s %s (correlation %.2f)\n", count,
                e.analysisRate(), e.hopSeconds() * 1000.0, wave::kKeyNames[key.key], key.minor ? "minor" : "major", key.score);
    if (midiPath) {
        if (!writeMidi(midiPath, all)) { std::fprintf(stderr, "cannot write %s\n", midiPath); return 1; }
        std::printf("wrote %s\n", midiPath);
    }
    return 0;
}
