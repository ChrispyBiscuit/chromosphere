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
    p.push_back(std::make_unique<F>(ParameterID{"erLevel", kV}, "ER / IR Level", NormalisableRange<float>(-60.f, 0.f, 0.1f, 2.f), -6.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<F>(ParameterID{"tailLevel", kV}, "Tail Level", NormalisableRange<float>(-60.f, 6.f, 0.1f, 2.f), 0.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<F>(ParameterID{"mix", kV}, "Mix", NormalisableRange<float>(0.f, 100.f, 0.1f), 30.f, juce::AudioParameterFloatAttributes().withLabel("%")));
    p.push_back(std::make_unique<F>(ParameterID{"outGain", kV}, "Output", NormalisableRange<float>(-24.f, 12.f, 0.1f), 0.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ParameterID{"quality", kV}, "Quality", juce::StringArray{"Eco", "Normal", "High"}, 1));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ParameterID{"diffusion", kV}, "Diffusion", true));
    // Convolution and Hybrid. Added in state version 2; sessions saved before load with these at their defaults.
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ParameterID{"mode", kV}, "Mode", juce::StringArray{"Algorithmic", "Convolution", "Hybrid"}, 0));
    p.push_back(std::make_unique<F>(ParameterID{"erMs", kV}, "Hybrid ER Length", NormalisableRange<float>(0.f, 150.f, 1.f), 0.f, juce::AudioParameterFloatAttributes().withLabel("ms (0 = auto)")));
    p.push_back(std::make_unique<F>(ParameterID{"tailScale", kV}, "Hybrid Tail Decay", NormalisableRange<float>(0.25f, 4.f, 0.01f, 0.5f), 1.f, juce::AudioParameterFloatAttributes().withLabel("x")));
    p.push_back(std::make_unique<F>(ParameterID{"irStart", kV}, "IR Start", NormalisableRange<float>(0.f, 2000.f, 1.f, 0.4f), 0.f, juce::AudioParameterFloatAttributes().withLabel("ms")));
    p.push_back(std::make_unique<F>(ParameterID{"irLength", kV}, "IR Length", NormalisableRange<float>(0.f, 30000.f, 10.f, 0.35f), 0.f, juce::AudioParameterFloatAttributes().withLabel("ms (0 = all)")));
    p.push_back(std::make_unique<F>(ParameterID{"irStretch", kV}, "IR Stretch", NormalisableRange<float>(0.5f, 2.f, 0.01f, 0.6f), 1.f, juce::AudioParameterFloatAttributes().withLabel("x")));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ParameterID{"irReverse", kV}, "IR Reverse", false));
    p.push_back(std::make_unique<F>(ParameterID{"irLowCut", kV}, "IR Low Cut", NormalisableRange<float>(0.f, 2000.f, 1.f, 0.3f), 0.f, juce::AudioParameterFloatAttributes().withLabel("Hz (0 = off)")));
    p.push_back(std::make_unique<F>(ParameterID{"irHighCut", kV}, "IR High Cut", NormalisableRange<float>(1000.f, 20000.f, 10.f, 0.4f), 20000.f, juce::AudioParameterFloatAttributes().withLabel("Hz (20k = off)")));
    p.push_back(std::make_unique<F>(ParameterID{"irWidth", kV}, "IR Width", NormalisableRange<float>(0.f, 2.f, 0.01f), 1.f));
    p.push_back(std::make_unique<F>(ParameterID{"irDecay", kV}, "IR Decay", NormalisableRange<float>(0.1f, 1.f, 0.01f, 0.6f), 1.f, juce::AudioParameterFloatAttributes().withLabel("x (1 = unchanged)")));
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
    mode_ = raw("mode"); erMs_ = raw("erMs"); tailScale_ = raw("tailScale");
    irStart_ = raw("irStart"); irLength_ = raw("irLength"); irStretch_ = raw("irStretch"); irReverse_ = raw("irReverse");
    irLowCut_ = raw("irLowCut"); irHighCut_ = raw("irHighCut"); irWidth_ = raw("irWidth"); irDecay_ = raw("irDecay");
}

FancyReverbProcessor::~FancyReverbProcessor() { host_.release(); }

fr::EngineSettings FancyReverbProcessor::readSettings() const {
    fr::EngineSettings s;
    s.quality = (fr::Quality)juce::jlimit(0, 2, (int)std::lround(quality_->load()));
    s.size = size_->load(); s.preDelayMs = pre_->load(); s.diffusion = diffusion_->load() > 0.5f;
    s.t60[0] = low_->load(); s.t60[1] = mid_->load(); s.t60[2] = high_->load();
    s.mode = (fr::EngineMode)juce::jlimit(0, 2, (int)std::lround(mode_->load()));
    s.erMs = erMs_->load(); s.tailScale = tailScale_->load();
    s.edit.startMs = irStart_->load(); s.edit.maxLengthMs = irLength_->load(); s.edit.stretch = irStretch_->load();
    s.edit.reverse = irReverse_->load() > 0.5f; s.edit.lowCutHz = irLowCut_->load();
    s.edit.highCutHz = irHighCut_->load() >= 19990.f ? 0.0 : irHighCut_->load();
    s.edit.width = irWidth_->load(); s.edit.decayScale = irDecay_->load();
    if (s.mode != fr::EngineMode::Algorithmic) { std::lock_guard<std::mutex> g(irMutex_); s.rawIr = ir_; }
    return s;
}

double FancyReverbProcessor::getTailLengthSeconds() const {
    double t = std::max({low_->load(), mid_->load(), high_->load()});
    if (mode_->load() > 0.5f) {
        std::shared_ptr<const fr::Ir> ir; { std::lock_guard<std::mutex> g(irMutex_); ir = ir_; }
        if (ir && ir->fs > 0) {
            const double len = ir->length() / ir->fs * irStretch_->load();
            t = mode_->load() > 1.5f ? std::max(len, (double)tailScale_->load() * 4.0) : len;   // Hybrid tails can be longer
        }
    }
    return t + pre_->load() * 0.001;
}

juce::String FancyReverbProcessor::loadImpulseResponse(const juce::File& file) {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if (!reader) return "Cannot read " + file.getFileName();
    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > 60 * (juce::int64)reader->sampleRate) return "IR is empty or longer than 60 s";
    const int nch = (int)reader->numChannels;
    juce::AudioBuffer<float> buf(nch, (int)reader->lengthInSamples);
    reader->read(&buf, 0, buf.getNumSamples(), 0, true, true);
    auto ir = std::make_shared<fr::Ir>();
    ir->fs = reader->sampleRate;
    const int use = nch >= 4 ? 4 : nch >= 2 ? 2 : 1;      // 3 channels: the first two
    for (int c = 0; c < use; ++c) ir->ch.emplace_back(buf.getReadPointer(c), buf.getReadPointer(c) + buf.getNumSamples());
    {
        std::lock_guard<std::mutex> g(irMutex_);
        ir_ = std::move(ir);
        irPath_ = file.getFullPathName();
    }
    apvts.state.setProperty("irPath", file.getFullPathName(), nullptr);
    return {};
}

void FancyReverbProcessor::clearImpulseResponse() {
    { std::lock_guard<std::mutex> g(irMutex_); ir_.reset(); irPath_.clear(); }
    apvts.state.setProperty("irPath", "", nullptr);
}

juce::String FancyReverbProcessor::getImpulseResponsePath() const { std::lock_guard<std::mutex> g(irMutex_); return irPath_; }

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
            // Version 1 had no mode or IR parameters: they load at their defaults (Algorithmic), so old
            // sessions sound the same. Later versions migrate here.
            apvts.replaceState(tree);
            const juce::String path = tree.getProperty("irPath").toString();
            if (path.isNotEmpty() && juce::File(path).existsAsFile()) loadImpulseResponse(juce::File(path));
            else clearImpulseResponse();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FancyReverbProcessor(); }
