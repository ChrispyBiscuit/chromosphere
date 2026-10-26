// Offline renderer: impulse or WAV in, 32-bit float stereo WAV out.
//   cs_render --impulse 8 --out ir.wav [--rate 48000] [--block 512]
//             [--mode algo|conv|hybrid|shimmer|redshift|corona|wormhole|eventhorizon|nebula|pulsar|solarwind|aurora|supernova] [--macro a b c] [--quality eco|normal|high] [--size 1] [--t60 3 2 1]
//             [--mod 1 0.7] [--er 0] [--tail 1] [--seed 9] [--predelay 0] [--no-diffusion]
//             [--ir file.wav] [--level 1] [--er-ms 80] [--tail-scale 1]
//             [--ir-start ms] [--ir-len ms] [--ir-fade ms] [--ir-reverse] [--ir-stretch x]
//             [--ir-lowcut Hz] [--ir-highcut Hz] [--ir-width x] [--ir-decay x]
//   cs_render --in dry.wav --out wet.wav ...
// In hybrid mode --t60 is ignored: the tail times come from the IR (times --tail-scale).
#include "engines.h"
#include "io/wav_io.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    cs::EngineSpec spec;
    cs::IrEdit edit;
    const char *in = nullptr, *out = nullptr, *irPath = nullptr;
    double impulseSec = 0, tailScale = 1.0; int rate = 48000, block = 512;
    float er = 0.f, tail = 1.f, level = 1.f;
    std::string mode = "algo";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto num = [&](int k) { return std::atof(argv[i + k]); };
        if (a == "--in") in = argv[++i];
        else if (a == "--out") out = argv[++i];
        else if (a == "--ir") irPath = argv[++i];
        else if (a == "--mode") mode = argv[++i];
        else if (a == "--impulse") impulseSec = num(1), ++i;
        else if (a == "--rate") rate = (int)num(1), ++i;
        else if (a == "--block") block = (int)num(1), ++i;
        else if (a == "--size") spec.reverb.size = num(1), ++i;
        else if (a == "--t60") { for (int k = 0; k < 3; ++k) spec.reverb.t60[k] = num(1 + k); i += 3; }
        else if (a == "--mod") { spec.reverb.modDepth = num(1); spec.reverb.modRateHz = num(2); i += 2; }
        else if (a == "--er") er = (float)num(1), ++i;
        else if (a == "--tail") tail = (float)num(1), ++i;
        else if (a == "--level") level = (float)num(1), ++i;
        else if (a == "--seed") spec.reverb.seed = (uint64_t)num(1), ++i;
        else if (a == "--predelay") spec.reverb.preDelayMs = num(1), ++i;
        else if (a == "--no-diffusion") spec.reverb.diffusion = false;
        else if (a == "--er-ms") spec.erMs = num(1), ++i;
        else if (a == "--macro") { for (int k = 0; k < 3; ++k) spec.macro[k] = num(1 + k); i += 3; }
        else if (a == "--tail-scale") tailScale = num(1), ++i;
        else if (a == "--ir-start") edit.startMs = num(1), ++i;
        else if (a == "--ir-len") edit.maxLengthMs = num(1), ++i;
        else if (a == "--ir-fade") edit.fadeOutMs = num(1), ++i;
        else if (a == "--ir-reverse") edit.reverse = true;
        else if (a == "--ir-stretch") edit.stretch = num(1), ++i;
        else if (a == "--ir-lowcut") edit.lowCutHz = num(1), ++i;
        else if (a == "--ir-highcut") edit.highCutHz = num(1), ++i;
        else if (a == "--ir-width") edit.width = num(1), ++i;
        else if (a == "--ir-decay") edit.decayScale = num(1), ++i;
        else if (a == "--quality") {
            std::string q = argv[++i];
            spec.reverb.quality = q == "eco" ? cs::Quality::Eco : q == "high" ? cs::Quality::High : cs::Quality::Normal;
        } else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
    }
    if (!out || (!in && impulseSec <= 0)) { std::fprintf(stderr, "need --out and (--in or --impulse)\n"); return 2; }
    cs::WavData src;
    if (in) {
        if (!cs::readWav(in, src) || src.ch.empty()) { std::fprintf(stderr, "cannot read %s\n", in); return 1; }
        rate = src.rate;
        if (src.ch.size() == 1) src.ch.push_back(src.ch[0]);
    } else {
        src.rate = rate; src.ch.assign(2, std::vector<float>((size_t)(impulseSec * rate), 0.f));
        src.ch[0][0] = src.ch[1][0] = 1.f;
    }
    spec.mode = mode == "conv" ? cs::EngineMode::Convolution : mode == "hybrid" ? cs::EngineMode::Hybrid : cs::EngineMode::Algorithmic;
    static const char* recipes[] = {"shimmer", "redshift", "corona", "wormhole", "eventhorizon", "nebula", "pulsar", "solarwind", "aurora", "supernova"};
    for (int k = 0; k < cs::kRecipeCount; ++k)
        if (mode == recipes[k]) { spec.mode = cs::EngineMode::Cosmic; spec.recipe = (cs::Recipe)k; }
    if (spec.mode == cs::EngineMode::Convolution || spec.mode == cs::EngineMode::Hybrid) {
        cs::WavData w;
        if (!irPath || !cs::readWav(irPath, w)) { std::fprintf(stderr, "need a readable --ir for mode %s\n", mode.c_str()); return 1; }
        cs::Ir ir; ir.fs = w.rate; ir.ch = w.ch;
        if (ir.ch.size() == 3 || ir.ch.size() > 4) ir.ch.resize(ir.ch.size() > 4 ? 4 : 2);
        ir = cs::resampleIr(ir, rate);
        ir = cs::editIr(ir, edit);
        spec.ir = std::make_shared<cs::Ir>(ir);
        if (spec.mode == cs::EngineMode::Hybrid) cs::hybridTailT60(ir, tailScale, spec.reverb.t60);
    }
    auto eng = cs::makeEngine(spec, rate, block);
    eng->setLevels((spec.mode == cs::EngineMode::Algorithmic || spec.mode == cs::EngineMode::Cosmic) ? er : level, tail, 0);
    const size_t n = src.ch[0].size();
    cs::WavData res; res.rate = rate; res.ch.assign(2, std::vector<float>(n));
    for (size_t off = 0; off < n; off += block) {   // host-style blocks
        int c = (int)std::min<size_t>(block, n - off);
        eng->process(&src.ch[0][off], &src.ch[1][off], &res.ch[0][off], &res.ch[1][off], c);
    }
    if (!cs::writeWav(out, res)) { std::fprintf(stderr, "cannot write %s\n", out); return 1; }
    return 0;
}
