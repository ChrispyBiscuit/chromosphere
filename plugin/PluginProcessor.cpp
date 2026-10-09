#include "PluginProcessor.h"
#include <cmath>

namespace {
using juce::NormalisableRange;
using juce::ParameterID;
constexpr int kV = 1;   // parameter version (AU/VST3 identity); bump only when a parameter is removed
}

juce::AudioProcessorValueTreeState::ParameterLayout FancyReverbProcessor::createLayout() {
    using F = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto sec = juce::AudioParameterFloatAttributes().withLabel("s");
    p.push_back(std::make_unique<F>(ParameterID{"size", kV}, "Size", NormalisableRange<float>(0.5f, 3.f, 0.001f, 0.6f), 1.f));
    p.push_back(std::make_unique<F>(ParameterID{"decayLow", kV}, "Decay Low", NormalisableRange<float>(0.1f, 30.f, 0.01f, 0.35f), 3.f, sec));
    p.push_back(std::make_unique<F>(ParameterID{"decayMid", kV}, "Decay Mid", NormalisableRange<float>(0.1f, 30.f, 0.01f, 0.35f), 2.f, sec));
    p.push_back(std::make_unique<F>(ParameterID{"decayHigh", kV}, "Decay High", NormalisableRange<float>(0.1f, 30.f, 0.01f, 0.35f), 1.f, sec));
    p.push_back(std::make_unique<F>(ParameterID{"preDelay", kV}, "Pre-delay", NormalisableRange<float>(0.f, 250.f, 0.1f, 0.5f), 0.f, juce::AudioParameterFloatAttributes().withLabel("ms")));
    p.push_back(std::make_unique<F>(ParameterID{"modDepth", kV}, "Mod Depth", NormalisableRange<float>(0.f, 4.f, 0.01f), 1.f));
    p.push_back(std::make_unique<F>(ParameterID{"modRate", kV}, "Mod Rate", NormalisableRange<float>(0.1f, 3.f, 0.01f, 0.5f), 0.7f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
    p.push_back(std::make_unique<F>(ParameterID{"erLevel", kV}, "ER Level", NormalisableRange<float>(-60.f, 0.f, 0.1f, 2.f), -6.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<F>(ParameterID{"tailLevel", kV}, "Tail Level", NormalisableRange<float>(-60.f, 6.f, 0.1f, 2.f), 0.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<F>(ParameterID{"mix", kV}, "Mix", NormalisableRange<float>(0.f, 100.f, 0.1f), 30.f, juce::AudioParameterFloatAttributes().withLabel("%")));
    p.push_back(std::make_unique<F>(ParameterID{"outGain", kV}, "Output", NormalisableRange<float>(-24.f, 12.f, 0.1f), 0.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ParameterID{"quality", kV}, "Quality", juce::StringArray{"Eco", "Normal", "High"}, 1));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ParameterID{"diffusion", kV}, "Diffusion", true));
    return {p.begin(), p.end()};
}

FancyReverbProcessor::FancyReverbProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout()),
      host_([this] { return readSettings(); }) {
    auto raw = [this](const char* id) { return apvts.getRawParameterValue(id); };
    size_ = raw("size"); low_ = raw("decayLow"); mid_ = raw("decayMid"); high_ = raw("decayHigh");
    pre_ = raw("preDelay"); modDepth_ = raw("modDepth"); modRate_ = raw("modRate");
    er_ = raw("erLevel"); tail_ = raw("tailLevel"); mix_ = raw("mix"); out_ = raw("outGain");
    quality_ = raw("quality"); diffusion_ = raw("diffusion");
}

FancyReverbProcessor::~FancyReverbProcessor() { host_.release(); }

fr::EngineSettings FancyReverbProcessor::readSettings() const {
    fr::EngineSettings s;
    s.quality = (fr::Quality)juce::jlimit(0, 2, (int)std::lround(quality_->load()));
    s.size = size_->load(); s.preDelayMs = pre_->load(); s.diffusion = diffusion_->load() > 0.5f;
    s.t60[0] = low_->load(); s.t60[1] = mid_->load(); s.t60[2] = high_->load();
    return s;
}

double FancyReverbProcessor::getTailLengthSeconds() const {
    return std::max({low_->load(), mid_->load(), high_->load()}) + pre_->load() * 0.001;
}

bool FancyReverbProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto out = l.getMainOutputChannelSet(), in = l.getMainInputChannelSet();
    return out == juce::AudioChannelSet::stereo() && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void FancyReverbProcessor::prepareToPlay(double fs, int maxBlock) {
    for (auto* v : {&inL_, &inR_, &wetL_, &wetR_}) v->assign(kChunk, 0.f);
    for (auto* s : {&dryGain_, &wetGain_, &outGain_}) s->reset(fs, 0.02);
    auto mixGains = [](float m, float& d, float& w) { d = std::cos(1.5707963f * m * 0.01f); w = std::sin(1.5707963f * m * 0.01f); };
    float d, w; mixGains(mix_->load(), d, w);
    dryGain_.setCurrentAndTargetValue(d); wetGain_.setCurrentAndTargetValue(w);
    outGain_.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(out_->load()));
    host_.prepare(fs, maxBlock);
    setLatencySamples(0);
}

void FancyReverbProcessor::releaseResources() { host_.release(); }

void FancyReverbProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buf.getNumSamples(), chans = buf.getNumChannels();
    if (chans < 1) return;
    for (int c = 2; c < chans; ++c) buf.clear(c, 0, n);
    if (chans < 2) return;   // layout check keeps this unreachable
    const bool monoIn = getTotalNumInputChannels() < 2;

    float d, w; const float m = mix_->load();
    d = std::cos(1.5707963f * m * 0.01f); w = std::sin(1.5707963f * m * 0.01f);
    dryGain_.setTargetValue(d); wetGain_.setTargetValue(w);
    outGain_.setTargetValue(juce::Decibels::decibelsToGain(out_->load()));
    const float er = juce::Decibels::decibelsToGain(er_->load(), -60.f);
    const float tail = juce::Decibels::decibelsToGain(tail_->load(), -60.f);

    float* L = buf.getWritePointer(0);
    float* R = buf.getWritePointer(1);
    for (int off = 0; off < n; off += kChunk) {
        const int c = std::min(kChunk, n - off);
        std::copy(L + off, L + off + c, inL_.begin());
        if (monoIn) std::copy(inL_.begin(), inL_.begin() + c, inR_.begin());
        else std::copy(R + off, R + off + c, inR_.begin());
        host_.process(inL_.data(), inR_.data(), wetL_.data(), wetR_.data(), c, er, tail, modDepth_->load(), modRate_->load());
        for (int i = 0; i < c; ++i) {
            const float dg = dryGain_.getNextValue(), wg = wetGain_.getNextValue(), og = outGain_.getNextValue();
            L[off + i] = og * (dg * inL_[i] + wg * wetL_[i]);
            R[off + i] = og * (dg * inR_[i] + wg * wetR_[i]);
        }
    }
}

juce::AudioProcessorEditor* FancyReverbProcessor::createEditor() { return new juce::GenericAudioProcessorEditor(*this); }

void FancyReverbProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = apvts.copyState();
    state.setProperty("stateVersion", kStateVersion, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void FancyReverbProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) {
            auto tree = juce::ValueTree::fromXml(*xml);
            // Version 1 is the only layout so far. Future versions migrate here, keeping the sound the same.
            apvts.replaceState(tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FancyReverbProcessor(); }
