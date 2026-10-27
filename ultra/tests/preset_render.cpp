// Renders every factory preset and checks it (PRD Section 11): every parameter id and choice name exists, the
// patterns parse, and four bars of every pattern that has steps play finite, under -0.1 dBFS after the limiter, loud
// enough (90th percentile of 50 ms RMS windows above -30 dBFS) and without DC.
// Usage: ultra_preset_render [--wav dir] [--measure file.json] [--only name]
//   --measure writes each preset's loudness (dBFS) for make_presets.py --calibrate.
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <map>

static int failures = 0;

namespace {
struct Result { float peak = 0, loud = -120, dc = 0; bool finite = true; int patterns = 0; };

Result renderPreset(UltraProcessor& proc, juce::AudioBuffer<float>* keep) {
    const double fs = 48000.0; const int block = 256;
    Result r;
    std::vector<float> mono;
    const auto& bank = proc.patterns.bank();
    for (int pat = 0; pat < ultra::kPatterns; ++pat) {
        bool any = false;
        for (const auto& tp : bank.patterns[(size_t)pat].tracks) for (int s = 0; s < tp.length; ++s) any |= tp.steps[(size_t)s].vel > 0;
        if (!any) continue;
        ++r.patterns;
        if (auto* prm = proc.apvts.getParameter("pattern")) prm->setValueNotifyingHost(prm->convertTo0to1((float)pat));
        proc.machine.reset();
        proc.setPlaying(true);
        const double bpm = proc.apvts.getRawParameterValue("tempo")->load();
        const int total = (int)(fs * 60.0 / bpm * 16.0);       // four bars
        juce::AudioBuffer<float> buf(2, block);
        for (int off = 0; off < total; off += block) {
            juce::MidiBuffer midi;
            proc.processBlock(buf, midi);
            for (int i = 0; i < block; ++i) {
                const float l = buf.getSample(0, i), rr = buf.getSample(1, i);
                if (!std::isfinite(l) || !std::isfinite(rr)) r.finite = false;
                r.peak = std::max(r.peak, std::max(std::abs(l), std::abs(rr)));
                mono.push_back(0.5f * (l + rr));
                if (keep) { keep->setSize(2, (int)mono.size(), true); keep->setSample(0, (int)mono.size() - 1, l); keep->setSample(1, (int)mono.size() - 1, rr); }
            }
        }
        proc.setPlaying(false);
    }
    if (auto* prm = proc.apvts.getParameter("pattern")) prm->setValueNotifyingHost(0.f);
    if (mono.empty()) return r;
    // Loudness: 90th percentile of 50 ms RMS windows.
    std::vector<float> win;
    const int W = (int)(fs * 0.05);
    double sum = 0, e2 = 0;
    for (size_t s = 0; s + (size_t)W <= mono.size(); s += (size_t)W) {
        double e = 0;
        for (int i = 0; i < W; ++i) e += mono[s + (size_t)i] * mono[s + (size_t)i];
        win.push_back((float)std::sqrt(e / W));
    }
    for (float v : mono) { sum += v; e2 += v * v; }
    std::sort(win.begin(), win.end());
    const float p90 = win.empty() ? 0.f : win[(size_t)(win.size() * 9 / 10)];
    r.loud = 20.f * std::log10(std::max(p90, 1e-6f));
    const double rms = std::sqrt(e2 / mono.size());
    r.dc = rms > 1e-6 ? (float)(std::abs(sum / mono.size()) / rms) : 0.f;
    return r;
}

bool checkIds(UltraProcessor& proc, const Preset& p) {
    bool ok = true;
    for (const auto& kv : p.params) {
        auto* prm = proc.apvts.getParameter(kv.name.toString());
        if (!prm) { std::printf("  unknown parameter %s\n", kv.name.toString().toRawUTF8()); ok = false; continue; }
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(prm))
            if (kv.value.isString() && c->choices.indexOf(kv.value.toString()) < 0) {
                std::printf("  %s: no choice named \"%s\"\n", kv.name.toString().toRawUTF8(), kv.value.toString().toRawUTF8()); ok = false;
            }
    }
    if (!p.patterns.isArray() || p.patterns.size() == 0) { std::printf("  no patterns\n"); ok = false; }
    if (!p.names.isArray() || p.names.size() != ultra::kTracks) { std::printf("  track names missing\n"); ok = false; }
    return ok;
}
}  // namespace

int main(int argc, char** argv) {
    setenv("ULTRA_NO_UPDATE_CHECK", "1", 1);
    juce::ScopedJuceInitialiser_GUI init;
    juce::File wavDir, measureFile;
    juce::String only;
    for (int i = 1; i + 1 < argc; ++i) {
        const juce::String a = argv[i];
        if (a == "--wav") wavDir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[++i]);
        else if (a == "--measure") measureFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[++i]);
        else if (a == "--only") only = argv[++i];
    }
    if (wavDir != juce::File()) wavDir.createDirectory();
    UltraProcessor proc;
    proc.setPlayConfigDetails(0, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    const auto& ps = proc.presets.presets();
    int factory = 0;
    std::map<juce::String, int> perCategory;
    auto* measured = new juce::DynamicObject();
    float loMin = 0, loMax = -120;
    for (int i = 0; i < (int)ps.size(); ++i) {
        const auto& p = ps[(size_t)i];
        if (!p.factory) continue;
        ++factory; ++perCategory[p.category];
        if (only.isNotEmpty() && p.name != only) continue;
        const bool idsOk = checkIds(proc, p);
        proc.presets.apply(i);
        juce::AudioBuffer<float> keep;
        const Result r = renderPreset(proc, wavDir != juce::File() ? &keep : nullptr);
        const bool ok = idsOk && r.finite && r.patterns > 0 && r.peak < 0.989f && r.loud > -30.f && r.dc < 0.01f;
        if (!ok) ++failures;
        loMin = std::min(loMin, r.loud); loMax = std::max(loMax, r.loud);
        std::printf("%s  %-24s %-16s patterns %d  peak %6.2f dBFS  loud %6.1f dBFS  dc %.3f\n", ok ? "PASS" : "FAIL", p.name.toRawUTF8(), p.category.toRawUTF8(),
                    r.patterns, 20.f * std::log10(std::max(r.peak, 1e-6f)), r.loud, r.dc);
        measured->setProperty(p.name, r.loud);
        if (wavDir != juce::File()) {
            juce::WavAudioFormat wav;
            auto f = wavDir.getChildFile(juce::File::createLegalFileName(p.name) + ".wav");
            f.deleteFile();
            if (auto w = std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(new juce::FileOutputStream(f), 48000.0, 2, 24, {}, 0)))
                w->writeFromAudioSampleBuffer(keep, 0, keep.getNumSamples());
        }
    }
    if (measureFile != juce::File()) measureFile.replaceWithText(juce::JSON::toString(juce::var(measured)));
    std::printf("\n%d factory presets in %d categories; loudness %.1f .. %.1f dBFS\n", factory, (int)perCategory.size(), loMin, loMax);
    if (only.isEmpty() && factory < 100) { std::printf("FAIL  fewer than 100 factory presets\n"); ++failures; }
    std::printf("%d failure(s)\n", failures);
    return failures;
}
