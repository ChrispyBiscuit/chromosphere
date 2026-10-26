#pragma once
// Impulse-response preparation and analysis. Offline: allocates and takes milliseconds to
// seconds, so call it on a worker thread, never on the audio thread.
#include <vector>

namespace cs {

struct Ir {
    double fs = 48000;
    std::vector<std::vector<float>> ch;     // 1, 2 or 4 channels (see PartitionedConvolver)
    size_t length() const { return ch.empty() ? 0 : ch[0].size(); }
};

// Longest IR the convolver is given, per channel, after resampling (30 s at 48 kHz, 10 s at 192 kHz).
constexpr size_t kMaxIrSamples = 2'000'000;

struct IrEdit {
    double startMs = 0, maxLengthMs = 0;    // trim; maxLength 0 = keep the whole IR
    double fadeOutMs = 20;                  // applied at the end after trimming
    bool reverse = false;
    double stretch = 1.0;                   // time scale, 0.25 .. 4 (2 = twice as long)
    double lowCutHz = 0, highCutHz = 0;     // 0 = off
    double width = 1.0;                     // 0 = mono, 1 = unchanged, 2 = wide (stereo and true-stereo IRs)
    double decayScale = 1.0;                // <= 1 shortens the decay with an exponential envelope
    bool normalize = true;                  // scale so the louder output has power gain targetPowerGain
    double targetPowerGain = 0.35;          // matches the algorithmic engine at default settings (measured in test_ir_tools)
};

// Windowed-sinc resampler (Kaiser, 32 zero crossings per side). ratio = outRate / inRate.
std::vector<float> resample(const std::vector<float>& x, double ratio);
Ir resampleIr(const Ir& in, double newFs);

// Apply the edits in a fixed order: trim, reverse, stretch, filters, width, decay, fade, normalize.
Ir editIr(const Ir& in, const IrEdit& e);

struct IrAnalysis {
    double t60[3] = {1, 1, 1};              // seconds at 125 Hz, 1 kHz, 8 kHz (T30 fits, clamped)
    double rms[3] = {0, 0, 0};              // not used yet
    size_t usableLength = 0;                // samples before the noise floor
    double energyDb(const std::vector<float>& x, size_t from, size_t to) const;
};
// Estimates decay times from the channel average. For IRs with a clear tail.
IrAnalysis analyzeIr(const Ir& ir);

// Mean power per sample of x in the octave band around fc (4th-order band-pass) over [from, to) samples.
double bandPower(const std::vector<float>& x, double fc, double fs, size_t from, size_t to);

// Mean power per sample over [fromMs, toMs), averaged over the IR channels.
double windowEnergy(const Ir& ir, double fromMs, double toMs);

}  // namespace cs
