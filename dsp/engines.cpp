#include "engines.h"
#include "common/delay_line.h"
#include "fdn/decay_design.h"
#include <algorithm>
#include <cmath>

namespace cs {
namespace {

struct Gain {
    float cur = 1, target = 1, step = 0; int left = 0;
    void set(float t, int n) { target = t; left = std::max(0, n); if (left == 0) cur = t; else step = (t - cur) / (float)left; }
    float next() { if (left > 0) { cur += step; if (--left == 0) cur = target; } return cur; }
};

class AlgorithmicEngine : public Engine {
public:
    AlgorithmicEngine(const ReverbParams& p, double fs, int maxBlock) {
        ReverbParams q = p; q.dry = 0.f; q.wet = 1.f;
        rv_.configure(q, fs, maxBlock);
    }
    void process(const float* a, const float* b, float* c, float* d, int n) override { rv_.process(a, b, c, d, n); }
    void setLevels(float er, float tail, int ramp) override { rv_.setMix(0.f, 1.f, er, tail, ramp); }
    void setModulation(double depth, double rate) override { rv_.setModulation(depth, rate); }
    void setDecay(const DecaySet& d, int ramp) override { rv_.setDecay(d, ramp); }
    void setFreeze(bool on, int ramp) override { rv_.setFreeze(on, ramp); }
    bool hasFdn() const override { return true; }
private:
    Reverb rv_;
};

class ConvolutionEngine : public Engine {
public:
    ConvolutionEngine(const EngineSpec& s, double fs, int maxBlock) : maxBlock_(maxBlock) {
        cv_.configure(s.ir->ch);
        pre_.configure((int)std::lround(s.reverb.preDelayMs * fs / 1000.0));
        tmpL_.assign(maxBlock, 0.f); tmpR_.assign(maxBlock, 0.f);
    }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n) override {
        for (int off = 0; off < n; off += maxBlock_) {
            const int c = std::min(maxBlock_, n - off);
            pre_.process(inL + off, inR + off, tmpL_.data(), tmpR_.data(), c);
            cv_.process(tmpL_.data(), tmpR_.data(), outL + off, outR + off, c);
            for (int i = 0; i < c; ++i) { const float g = gain_.next(); outL[off + i] *= g; outR[off + i] *= g; }
        }
    }
    void setLevels(float er, float, int ramp) override { gain_.set(er, ramp); }
    void setModulation(double, double) override {}
    void setDecay(const DecaySet&, int) override {}
    void setFreeze(bool, int) override {}
    bool hasFdn() const override { return false; }
private:
    int maxBlock_;
    PartitionedConvolver cv_;
    DelayLine pre_;
    Gain gain_;
    std::vector<float> tmpL_, tmpR_;
};

class HybridEngine : public Engine {
public:
    HybridEngine(const EngineSpec& s, double fs, int maxBlock) : maxBlock_(maxBlock) {
        const Ir& ir = *s.ir;
        if (s.erMs <= 0) {   // auto: a fifth of the mid decay time, between 20 and 80 ms
            const double t = analyzeIr(ir).t60[1];
            erMs_ = std::clamp(0.2 * t * 1000.0, 20.0, 80.0);
        } else erMs_ = s.erMs;
        // Early part: first erMs of the IR with a raised-cosine fade over the last quarter (at least 5 ms).
        const size_t erN = std::min(ir.length(), (size_t)(erMs_ * fs / 1000.0));
        const size_t fadeN = std::min(erN, std::max((size_t)(0.005 * fs), erN / 4));
        std::vector<std::vector<float>> early;
        for (const auto& c : ir.ch) {
            std::vector<float> e(c.begin(), c.begin() + erN);
            for (size_t i = 0; i < fadeN; ++i) e[erN - fadeN + i] *= (float)(0.5 + 0.5 * std::cos(3.14159265358979 * (i + 1.0) / fadeN));
            early.push_back(std::move(e));
        }
        cv_.configure(early);
        pre_.configure((int)std::lround(s.reverb.preDelayMs * fs / 1000.0));
        // Tail: FDN fed after erMs - 30 ms. ER stage off. Level matched to the IR.
        ReverbParams tp = s.reverb; tp.dry = 0.f; tp.wet = 1.f; tp.erLevel = 0.f; tp.tailLevel = 1.f; tp.preDelayMs = 0.0;
        tail_.configure(tp, fs, maxBlock);
        const double tailDelayMs = std::max(0.0, erMs_ - 30.0);
        tailDelay_.configure((int)std::lround(tailDelayMs * fs / 1000.0));
        const Shape shape = matchLevel(ir, tp, fs, erMs_, tailDelayMs);
        matchGain_ = shape.mid;
        lowSh_ = makeShelf(true, shape.low, 400.0, fs); highSh_ = makeShelf(false, shape.high, 3000.0, fs);
        tmpL_.assign(maxBlock, 0.f); tmpR_.assign(maxBlock, 0.f); tL_.assign(maxBlock, 0.f); tR_.assign(maxBlock, 0.f);
        dL_.assign(maxBlock, 0.f); dR_.assign(maxBlock, 0.f);
        erGain_.set(1.f, 0); tailGain_.set(matchGain_, 0);
    }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n) override {
        for (int off = 0; off < n; off += maxBlock_) {
            const int c = std::min(maxBlock_, n - off);
            pre_.process(inL + off, inR + off, tmpL_.data(), tmpR_.data(), c);
            for (int i = 0; i < c; ++i) { const float g = inGate_.next(); tmpL_[i] *= g; tmpR_[i] *= g; }
            cv_.process(tmpL_.data(), tmpR_.data(), dL_.data(), dR_.data(), c);
            tailDelay_.process(tmpL_.data(), tmpR_.data(), tL_.data(), tR_.data(), c);
            tail_.process(tL_.data(), tR_.data(), tL_.data(), tR_.data(), c);
            for (int i = 0; i < c; ++i) {
                const float e = erGain_.next(), t = tailGain_.next();
                float* tc[2] = {&tL_[i], &tR_[i]};
                for (int ch = 0; ch < 2; ++ch) {          // spectral match of the tail: low and high shelves
                    float x = *tc[ch];
                    float y = (float)lowSh_.b0 * x + st_[ch][0]; st_[ch][0] = (float)lowSh_.b1 * x - (float)lowSh_.a1 * y;
                    float z = (float)highSh_.b0 * y + st_[ch][1]; st_[ch][1] = (float)highSh_.b1 * y - (float)highSh_.a1 * z;
                    *tc[ch] = z;
                }
                outL[off + i] = e * dL_[i] + t * tL_[i];
                outR[off + i] = e * dR_[i] + t * tR_[i];
            }
        }
    }
    void setLevels(float er, float tail, int ramp) override { erGain_.set(er, ramp); tailGain_.set(tail * matchGain_, ramp); }
    void setModulation(double depth, double rate) override { tail_.setModulation(depth, rate); }
    void setDecay(const DecaySet& d, int ramp) override { tail_.setDecay(d, ramp); }
    void setFreeze(bool on, int ramp) override { tail_.setFreeze(on, ramp); inGate_.set(on ? 0.f : 1.f, ramp); }
    bool hasFdn() const override { return true; }

private:
    struct Shape { float mid = 1.f, low = 1.f, high = 1.f; };
    // Gains that make the FDN tail's band powers (125 Hz, 1 kHz, 8 kHz octaves) just after the early
    // part equal the IR's. mid scales the tail; low and high become shelves relative to mid.
    static Shape matchLevel(const Ir& ir, const ReverbParams& tp, double fs, double erMs, double tailDelayMs) {
        // Short window once the FDN has built up (60 ms after the early part; length a seventh of the
        // decay time, 15..100 ms) so the match is not skewed by the two tails decaying at different rates.
        const double w0 = erMs + 60.0, w1 = w0 + std::clamp(0.15 * tp.t60[1] * 1000.0, 15.0, 100.0);
        Reverb probe; probe.configure(tp, fs, 1024);
        const size_t len = (size_t)((w1 + 50.0) * fs / 1000.0);
        std::vector<float> in(len, 0.f), l(len), r(len);
        in[0] = 1.f;
        probe.process(in.data(), in.data(), l.data(), r.data(), (int)len);   // impulse on both inputs, like a centred source
        // The engine feeds the tail tailDelayMs late, so IR time t corresponds to probe time t - tailDelayMs.
        const size_t p0 = (size_t)((w0 - tailDelayMs) * fs / 1000.0), p1 = std::min(len, (size_t)((w1 - tailDelayMs) * fs / 1000.0));
        const size_t q0 = (size_t)(w0 * fs / 1000.0), q1 = (size_t)(w1 * fs / 1000.0);
        const double centres[3] = {125.0, 1000.0, 8000.0};
        float g[3];
        for (int b = 0; b < 3; ++b) {
            double pf = 0.5 * (bandPower(l, centres[b], fs, p0, p1) + bandPower(r, centres[b], fs, p0, p1)), pi = 0;
            for (const auto& c : ir.ch) pi += bandPower(c, centres[b], fs, q0, std::min(q1, c.size()));
            pi /= ir.ch.size();
            g[b] = pf > 0 && pi > 0 ? (float)std::clamp(std::sqrt(pi / pf), 0.03, 30.0) : 1.f;   // clamp: +-30 dB
        }
        Shape s; s.mid = g[1]; s.low = g[0] / g[1]; s.high = g[2] / g[1];
        return s;
    }
    int maxBlock_;
    double erMs_ = 80;
    PartitionedConvolver cv_;
    DelayLine pre_, tailDelay_;
    Reverb tail_;
    Gain erGain_, tailGain_, inGate_;
    float matchGain_ = 1.f;
    Shelf lowSh_{1, 0, 0}, highSh_{1, 0, 0};
    float st_[2][2] = {};          // [channel][low, high] filter states
    std::vector<float> tmpL_, tmpR_, tL_, tR_, dL_, dR_;
};

class CosmicEngineAdapter : public Engine {
public:
    CosmicEngineAdapter(const EngineSpec& s, double fs, int maxBlock) {
        eng_.configure(s.recipe, s.reverb, fs, maxBlock);
        eng_.setMacros(s.macro[0], s.macro[1], s.macro[2]);
    }
    void process(const float* a, const float* b, float* c, float* d, int n) override { eng_.process(a, b, c, d, n); }
    void setLevels(float er, float tail, int ramp) override { eng_.setLevels(er, tail, ramp); }
    void setModulation(double depth, double rate) override { if (eng_.recipe() != Recipe::Corona) eng_.setModulation(depth, rate); }
    void setDecay(const DecaySet& d, int ramp) override { eng_.setDecay(d, ramp); }
    void setFreeze(bool on, int ramp) override { eng_.setFreeze(on, ramp); }
    void setMacros(double a, double b, double c) override { eng_.setMacros(a, b, c); }
    bool hasFdn() const override { return true; }
private:
    CosmicEngine eng_;
};

class VintageEngineAdapter : public Engine {
public:
    VintageEngineAdapter(const EngineSpec& s, double fs, int maxBlock) {
        eng_.configure(s.unit, s.reverb, fs, maxBlock);
        eng_.setMacros(s.macro[0], s.macro[1], s.macro[2]);
    }
    void process(const float* a, const float* b, float* c, float* d, int n) override { eng_.process(a, b, c, d, n); }
    void setLevels(float er, float tail, int ramp) override { eng_.setLevels(er, tail, ramp); }
    void setModulation(double depth, double rate) override { eng_.setModulation(depth, rate); }
    void setDecay(const DecaySet& d, int ramp) override { eng_.setDecay(d, ramp); }
    void setFreeze(bool on, int ramp) override { eng_.setFreeze(on, ramp); }
    void setMacros(double a, double b, double c) override { eng_.setMacros(a, b, c); }
    bool hasFdn() const override { return true; }   // the host designs decay sets; non-FDN units read t60 from them
private:
    VintageEngine eng_;
};

class AcousticEngineAdapter : public Engine {
public:
    AcousticEngineAdapter(const EngineSpec& s, double fs, int maxBlock) { eng_.configure(spaceParamsFor(s), fs, maxBlock); }
    void process(const float* a, const float* b, float* c, float* d, int n) override { eng_.process(a, b, c, d, n); }
    void setLevels(float er, float tail, int ramp) override { eng_.setLevels(er, tail, ramp); }
    void setModulation(double depth, double rate) override { eng_.setModulation(depth, rate); }
    void setDecay(const DecaySet& d, int ramp) override { eng_.setDecay(d, ramp); }
    void setFreeze(bool on, int ramp) override { eng_.setFreeze(on, ramp); }
    void setDirect(float g, int ramp) override { eng_.setDirect(g, ramp); }
    bool hasFdn() const override { return true; }
private:
    SpaceEngine eng_;
};

}  // namespace

SpaceParams spaceParamsFor(const EngineSpec& spec) {
    SpaceParams p;
    p.room = spec.room;
    p.room.quality = spec.reverb.quality == Quality::Eco ? 0 : spec.reverb.quality == Quality::High ? 2 : 1;
    p.room.scattering = spec.reverb.diffusion;
    p.lines = spec.reverb.quality == Quality::Eco ? 16 : spec.reverb.quality == Quality::Normal ? 32 : 64;
    p.preDelayMs = spec.reverb.preDelayMs;
    p.seed = spec.reverb.seed;
    return p;
}

const char* modeName(int m) {
    if (m == kAcousticMode) return "Acoustic";
    if (m >= kFirstVintageMode && m < kAcousticMode) return unitName((Unit)(m - kFirstVintageMode));
    if (m >= kFirstCosmicMode && m < kFirstVintageMode) return recipeName((Recipe)(m - kFirstCosmicMode));
    return m == 1 ? "Convolution" : m == 2 ? "Hybrid" : "Algorithmic";
}
std::array<const char*, 3> modeMacroNames(int m) {
    if (m >= kFirstVintageMode && m < kAcousticMode) return unitMacroNames((Unit)(m - kFirstVintageMode));
    if (m >= kFirstCosmicMode && m < kFirstVintageMode) return recipeMacroNames((Recipe)(m - kFirstCosmicMode));
    return {"A", "B", "C"};
}

void hybridTailT60(const Ir& ir, double scale, double out[3]) {
    IrAnalysis a = analyzeIr(ir);
    for (int i = 0; i < 3; ++i) out[i] = std::clamp(a.t60[i] * scale, 0.1, 30.0);
}

std::unique_ptr<Engine> makeEngine(const EngineSpec& spec, double fs, int maxBlock) {
    const bool haveIr = spec.ir && spec.ir->length() > 0;
    if (spec.mode == EngineMode::Cosmic) return std::make_unique<CosmicEngineAdapter>(spec, fs, maxBlock);
    if (spec.mode == EngineMode::Vintage) return std::make_unique<VintageEngineAdapter>(spec, fs, maxBlock);
    if (spec.mode == EngineMode::Acoustic) return std::make_unique<AcousticEngineAdapter>(spec, fs, maxBlock);
    if (spec.mode == EngineMode::Convolution && haveIr) return std::make_unique<ConvolutionEngine>(spec, fs, maxBlock);
    if (spec.mode == EngineMode::Hybrid && haveIr) {
        // A tail of 0.6 s or less is mostly inside the early part already; the matched FDN tail adds nothing
        // and cannot follow such a fast decay, so the whole IR is convolved instead (cheap for a short IR).
        if (analyzeIr(*spec.ir).t60[1] < kHybridMinT60) return std::make_unique<ConvolutionEngine>(spec, fs, maxBlock);
        return std::make_unique<HybridEngine>(spec, fs, maxBlock);
    }
    return std::make_unique<AlgorithmicEngine>(spec.reverb, fs, maxBlock);
}

DecaySet designEngineDecay(const EngineSpec& spec, double fs) {
    const bool haveIr = spec.ir && spec.ir->length() > 0;
    if (spec.mode == EngineMode::Convolution && haveIr) return {};
    if (spec.mode == EngineMode::Acoustic) return SpaceEngine::designDecay(spaceParamsFor(spec), fs);
    ReverbParams p = spec.mode == EngineMode::Vintage ? unitReverbParams(spec.unit, spec.reverb) : spec.reverb;
    return Reverb::designDecay(p, fs);
}

}  // namespace cs
