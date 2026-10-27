// Renders every factory preset on its test material and checks it (PRD sections 6 and 8):
//   every parameter id, choice name and macro target in the preset exists; output finite; with the clip guard on,
//   peak below -0.1 dBFS; loudness (auto-gain off, K-weighted) within 1 dB of the input's after calibration;
//   at least 120 presets.
// Usage: muon_preset_render [--wav dir] [--measure file.json] [--only name] [--tolerance dB]
//   --measure writes each preset's loudness change (output minus input, dB) for make_presets.py --calibrate.
#include "PluginProcessor.h"
#include "material.h"
#include <MuonData.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <map>

static int failures = 0;

namespace {
struct Result { float peak = 0; double delta = 0; bool finite = true; };

Result renderPreset(MuonProcessor& proc, const material::Stereo& in, juce::AudioBuffer<float>* keep) {
    const double fs = 48000.0; const int block = 256;
    const int total = (int)in.L.size();
    juce::AudioBuffer<float> buf(2, block);
    material::Stereo out; out.L.assign((size_t)total, 0.f); out.R.assign((size_t)total, 0.f);
    if (keep) keep->setSize(2, total);
    Result r;
    for (int off = 0; off < total; off += block) {
        const int n = std::min(block, total - off);
        buf.setSize(2, n, false, false, true);
        for (int i = 0; i < n; ++i) { buf.setSample(0, i, in.L[(size_t)(off + i)]); buf.setSample(1, i, in.R[(size_t)(off + i)]); }
        juce::MidiBuffer midi;
        proc.processBlock(buf, midi);
        for (int i = 0; i < n; ++i) {
            const float l = buf.getSample(0, i), rr = buf.getSample(1, i);
            if (!std::isfinite(l) || !std::isfinite(rr)) r.finite = false;
            r.peak = std::max(r.peak, std::max(std::abs(l), std::abs(rr)));
            out.L[(size_t)(off + i)] = l; out.R[(size_t)(off + i)] = rr;
            if (keep) { keep->setSample(0, off + i, l); keep->setSample(1, off + i, rr); }
        }
    }
    const size_t from = (size_t)(0.5 * fs);
    r.delta = material::loudness(out.L, out.R, fs, from) - material::loudness(in.L, in.R, fs, from);
    return r;
}

bool checkIds(MuonProcessor& proc, const Preset& p) {
    bool ok = true;
    for (const auto& kv : p.params) {
        auto* prm = proc.apvts.getParameter(kv.name.toString());
        if (!prm) { std::printf("  unknown parameter %s\n", kv.name.toString().toRawUTF8()); ok = false; continue; }
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(prm))
            if (kv.value.isString() && c->choices.indexOf(kv.value.toString()) < 0) {
                std::printf("  %s: no choice named \"%s\"\n", kv.name.toString().toRawUTF8(), kv.value.toString().toRawUTF8()); ok = false;
            }
    }
    for (const auto& m : MuonProcessor::macrosFromVar(p.macros)) {
        auto* prm = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter(m.paramId));
        if (!prm || m.macro < 0 || m.macro >= MuonProcessor::kNumMacros) { std::printf("  bad macro target %d %s\n", m.macro, m.paramId.toRawUTF8()); ok = false; }
    }
    return ok;
}
}  // namespace

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    juce::File wavDir, measure; juce::String only;
    double tolerance = 1.0;
    for (int i = 1; i + 1 < argc; i += 2) {
        const juce::String a(argv[i]);
        if (a == "--wav") wavDir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[i + 1]);
        else if (a == "--measure") measure = juce::File::getCurrentWorkingDirectory().getChildFile(argv[i + 1]);
        else if (a == "--only") only = argv[i + 1];
        else if (a == "--tolerance") tolerance = juce::String(argv[i + 1]).getDoubleValue();
    }
    if (wavDir != juce::File()) wavDir.createDirectory();
    // The material each preset is written for, from the embedded JSON.
    std::map<juce::String, juce::String> materialOf;
    const juce::var root = juce::JSON::parse(juce::String::fromUTF8(MuonData::presets_json, MuonData::presets_jsonSize));
    if (auto* list = root["presets"].getArray()) for (auto& p : *list) materialOf[p["name"].toString()] = p["material"].toString();
    std::map<juce::String, material::Stereo> cache;

    MuonProcessor proc;
    proc.setPlayConfigDetails(2, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    const auto presets = proc.presets.presets();
    juce::DynamicObject::Ptr levels = new juce::DynamicObject();
    int count = 0, levelMisses = 0;
    std::map<juce::String, int> perCategory;
    for (int i = 0; i < (int)presets.size(); ++i) {
        const auto& p = presets[(size_t)i];
        if (!p.factory || (only.isNotEmpty() && p.name != only)) continue;
        ++count; ++perCategory[p.category];
        const bool idsOk = checkIds(proc, p);
        proc.presets.apply(i);
        if (auto* ag = proc.apvts.getParameter("autoGain")) ag->setValueNotifyingHost(0.f);   // measure the preset itself
        proc.engine.setParams(proc.currentParams());
        proc.engine.reset();
        const juce::String kind = materialOf.count(p.name) ? materialOf[p.name] : juce::String("mix");
        if (!cache.count(kind)) cache[kind] = material::make(kind.toStdString());
        juce::AudioBuffer<float> keep;
        const Result r = renderPreset(proc, cache[kind], wavDir != juce::File() ? &keep : nullptr);
        const bool guard = proc.apvts.getRawParameterValue("clipGuard")->load() > 0.5f;
        const bool levelOk = std::abs(r.delta) <= tolerance;
        if (!levelOk) ++levelMisses;
        const bool ok = idsOk && r.finite && (!guard || r.peak < 0.989f) && levelOk;
        if (!ok) ++failures;
        std::printf("%s  %-22s %-18s %-7s peak %6.1f dB  level %+5.1f dB\n", ok ? "PASS" : "FAIL", p.name.toRawUTF8(), p.category.toRawUTF8(),
                    kind.toRawUTF8(), 20.f * std::log10(std::max(r.peak, 1e-6f)), r.delta);
        levels->setProperty(p.name, std::round(r.delta * 100.0) / 100.0);
        if (wavDir != juce::File()) {
            juce::WavAudioFormat fmt;
            auto f = wavDir.getChildFile(juce::File::createLegalFileName(p.name) + ".wav");
            f.deleteFile();
            std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::FileOutputStream>(f);
            if (auto out = fmt.createWriterFor(os, juce::AudioFormatWriterOptions{}.withSampleRate(48000.0).withNumChannels(2).withBitsPerSample(24)))
                out->writeFromAudioSampleBuffer(keep, 0, keep.getNumSamples());
        }
    }
    if (measure != juce::File()) measure.replaceWithText(juce::JSON::toString(juce::var(levels.get())));
    std::printf("%d factory presets rendered", count);
    for (auto& [c, n] : perCategory) std::printf(", %s %d", c.toRawUTF8(), n);
    std::printf("\n%d outside +-%.1f dB of the input loudness\n%d failure(s)\n", levelMisses, tolerance, failures);
    if (only.isEmpty() && count < 120) { std::printf("FAIL  fewer than 120 factory presets\n"); ++failures; }
    return failures;
}
