// Renders every factory preset with a fixed phrase and checks it (PRD section 8):
//   every parameter id and choice name in the preset exists, output finite, peak under the limiter ceiling,
//   loud enough to hear, no DC offset.
// Usage: infra_preset_render [--wav dir] [--measure file.json] [--only name[,name...]]
//   --measure writes each preset's loudness (90th percentile of 50 ms RMS windows, dBFS) for make_presets.py --calibrate.
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <map>

static int failures = 0;

namespace {
struct Result { float peak = 0, loud = -120, dc = 0, minLimiter = 1; bool finite = true; };

Result renderPreset(InfraProcessor& proc, juce::AudioBuffer<float>* keep) {
    const double fs = 48000.0; const int block = 256;
    const int total = (int)(fs * 4.0);
    struct Ev { int t; bool on; int note; };
    std::vector<Ev> evs;
    for (int n : {48, 52, 55, 59}) { evs.push_back({0, true, n}); evs.push_back({(int)(fs * 1.6), false, n}); }
    const int melody[] = {60, 63, 67, 70, 72, 67};
    for (int i = 0; i < 6; ++i) { const int t = (int)(fs * (2.2 + 0.25 * i)); evs.push_back({t, true, melody[i]}); evs.push_back({t + (int)(fs * 0.22), false, melody[i]}); }
    juce::AudioBuffer<float> buf(2, block);
    if (keep) keep->setSize(2, total);
    Result r;
    std::vector<float> mono((size_t)total);
    for (int off = 0; off < total; off += block) {
        juce::MidiBuffer midi;
        for (const auto& e : evs)
            if (e.t >= off && e.t < off + block)
                midi.addEvent(e.on ? juce::MidiMessage::noteOn(1, e.note, (juce::uint8)100) : juce::MidiMessage::noteOff(1, e.note), e.t - off);
        proc.processBlock(buf, midi);
        // The output limiter hides a runaway signal behind -1 dBFS: watch how hard it has to work.
        r.minLimiter = std::min(r.minLimiter, proc.synth.limiterGain.load());
        for (int i = 0; i < block && off + i < total; ++i) {
            const float l = buf.getSample(0, i), rr = buf.getSample(1, i);
            if (!std::isfinite(l) || !std::isfinite(rr)) r.finite = false;
            r.peak = std::max(r.peak, std::max(std::abs(l), std::abs(rr)));
            mono[(size_t)(off + i)] = 0.5f * (l + rr);
            if (keep) { keep->setSample(0, off + i, l); keep->setSample(1, off + i, rr); }
        }
    }
    // Loudness: 90th percentile of 50 ms RMS windows over the played part.
    std::vector<float> win;
    const int W = (int)(fs * 0.05);
    double sum = 0;
    for (int s = 0; s + W <= (int)(fs * 3.8); s += W) {
        double e = 0;
        for (int i = 0; i < W; ++i) { e += mono[(size_t)(s + i)] * mono[(size_t)(s + i)]; sum += mono[(size_t)(s + i)]; }
        win.push_back((float)std::sqrt(e / W));
    }
    std::sort(win.begin(), win.end());
    const float p90 = win.empty() ? 0.f : win[(size_t)(win.size() * 9 / 10)];
    r.loud = 20.f * std::log10(std::max(p90, 1e-6f));
    double rms = 0; for (float v : mono) rms += v * v;
    rms = std::sqrt(rms / mono.size());
    r.dc = rms > 1e-6 ? (float)(std::abs(sum / (double)(win.size() * W)) / rms) : 0.f;
    return r;
}

bool checkIds(InfraProcessor& proc, const Preset& p) {
    bool ok = true;
    for (const auto& kv : p.params) {
        auto* prm = proc.apvts.getParameter(kv.name.toString());
        if (!prm) { std::printf("  unknown parameter %s\n", kv.name.toString().toRawUTF8()); ok = false; continue; }
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(prm))
            if (kv.value.isString() && c->choices.indexOf(kv.value.toString()) < 0) {
                std::printf("  %s: no choice named \"%s\"\n", kv.name.toString().toRawUTF8(), kv.value.toString().toRawUTF8()); ok = false;
            }
    }
    return ok;
}
}  // namespace

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    juce::File wavDir, measure; juce::String only;
    for (int i = 1; i + 1 < argc; i += 2) {
        const juce::String a(argv[i]);
        if (a == "--wav") wavDir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[i + 1]);
        else if (a == "--measure") measure = juce::File::getCurrentWorkingDirectory().getChildFile(argv[i + 1]);
        else if (a == "--only") only = argv[i + 1];
    }
    if (wavDir != juce::File()) wavDir.createDirectory();
    InfraProcessor proc;
    proc.setPlayConfigDetails(0, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    const auto presets = proc.presets.presets();
    juce::DynamicObject::Ptr levels = new juce::DynamicObject();
    int count = 0;
    std::map<juce::String, int> perCategory;
    for (int i = 0; i < (int)presets.size(); ++i) {
        const auto& p = presets[(size_t)i];
        if (!p.factory || (only.isNotEmpty() && !juce::StringArray::fromTokens(only, ",", "").contains(p.name))) continue;
        ++count; ++perCategory[p.category];
        const bool idsOk = checkIds(proc, p);
        proc.presets.apply(i);
        proc.synth.reset();
        juce::AudioBuffer<float> keep;
        const Result r = renderPreset(proc, wavDir != juce::File() ? &keep : nullptr);
        const bool ok = idsOk && r.finite && r.peak <= 0.9f && r.loud > -40.f && r.dc < 0.05f && r.minLimiter > 0.1f;   // more than 20 dB of limiting means a runaway signal
        if (!ok) ++failures;
        std::printf("%s  %-22s %-14s peak %6.1f dB  loud %6.1f dB  dc %4.1f%%  limiter %5.1f dB\n", ok ? "PASS" : "FAIL", p.name.toRawUTF8(), p.category.toRawUTF8(),
                    20.f * std::log10(std::max(r.peak, 1e-6f)), r.loud, r.dc * 100.f, 20.f * std::log10(std::max(r.minLimiter, 1e-6f)));
        levels->setProperty(p.name, std::round(r.loud * 10.f) / 10.f);
        if (wavDir != juce::File()) {
            juce::WavAudioFormat fmt;
            auto f = wavDir.getChildFile(juce::File::createLegalFileName(p.name) + ".wav");
            f.deleteFile();
            if (auto out = std::unique_ptr<juce::AudioFormatWriter>(fmt.createWriterFor(new juce::FileOutputStream(f), 48000.0, 2, 24, {}, 0)))
                out->writeFromAudioSampleBuffer(keep, 0, keep.getNumSamples());
        }
    }
    if (measure != juce::File()) measure.replaceWithText(juce::JSON::toString(juce::var(levels.get())));
    std::printf("%d factory presets rendered", count);
    for (auto& [c, n] : perCategory) std::printf(", %s %d", c.toRawUTF8(), n);
    std::printf("\n%d failure(s)\n", failures);
    if (only.isEmpty() && count < 100) { std::printf("FAIL  fewer than 100 factory presets\n"); ++failures; }
    return failures;
}
