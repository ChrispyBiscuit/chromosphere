#include "PatternStore.h"

using namespace ultra;

namespace {
const juce::Identifier idPatterns("PATTERNS"), idPattern("PATTERN"), idTrack("TRACK"), idNames("NAMES"), idLen("len"), idRate("rate");
const char* kDefaultNames[kTracks] = {"Kick", "Snare", "Clap", "Closed Hat", "Open Hat", "Low Tom", "High Tom", "Metal"};
}  // namespace

const char* PatternStore::laneKey(Lane l) {
    static const char* k[NumLanes] = {"v", "c", "r", "p", "n", "k", "l"};
    return k[l];
}
int PatternStore::laneMin(Lane l) { static const int v[NumLanes] = {0, 0, 1, -24, -50, 0, -100}; return v[l]; }
int PatternStore::laneMax(Lane l) { static const int v[NumLanes] = {127, 100, 4, 24, 50, kNumConds - 1, 100}; return v[l]; }
int PatternStore::laneDefault(Lane l) { static const int v[NumLanes] = {0, 100, 1, 0, 0, 0, 0}; return v[l]; }

static int getStep(const Step& s, PatternStore::Lane l) {
    switch (l) {
        case PatternStore::Vel: return s.vel;
        case PatternStore::Chance: return s.chance;
        case PatternStore::Ratchet: return s.ratchet;
        case PatternStore::Pitch: return s.pitch;
        case PatternStore::Nudge: return s.nudge;
        case PatternStore::Cond: return s.cond;
        case PatternStore::Lock: return s.lock;
        case PatternStore::NumLanes: break;
    }
    return 0;
}
static void setStep(Step& s, PatternStore::Lane l, int v) {
    v = juce::jlimit(PatternStore::laneMin(l), PatternStore::laneMax(l), v);
    switch (l) {
        case PatternStore::Vel: s.vel = (uint8_t)v; break;
        case PatternStore::Chance: s.chance = (uint8_t)v; break;
        case PatternStore::Ratchet: s.ratchet = (uint8_t)v; break;
        case PatternStore::Pitch: s.pitch = (int8_t)v; break;
        case PatternStore::Nudge: s.nudge = (int8_t)v; break;
        case PatternStore::Cond: s.cond = (uint8_t)v; break;
        case PatternStore::Lock: s.lock = (int8_t)v; break;
        case PatternStore::NumLanes: break;
    }
}

juce::String PatternStore::laneToString(const TrackPattern& tp, Lane l) {
    int last = -1;
    for (int s = 0; s < kMaxSteps; ++s) if (getStep(tp.steps[(size_t)s], l) != laneDefault(l)) last = s;
    juce::String out;
    for (int s = 0; s <= last; ++s) { if (s) out << ","; out << getStep(tp.steps[(size_t)s], l); }
    return out;
}

void PatternStore::laneFromString(TrackPattern& tp, Lane l, const juce::String& str) {
    juce::StringArray parts; parts.addTokens(str, ",", "");
    for (int s = 0; s < kMaxSteps; ++s)
        setStep(tp.steps[(size_t)s], l, s < parts.size() && parts[s].trim().isNotEmpty() ? parts[s].getIntValue() : laneDefault(l));
}

juce::String PatternStore::defaultTrackName(int t) { return kDefaultNames[juce::jlimit(0, kTracks - 1, t)]; }

PatternStore::PatternStore(juce::AudioProcessorValueTreeState& state, juce::UndoManager* undo) : apvts_(state), undo_(undo) {
    buffers_ = std::make_unique<std::array<PatternBank, 3>>();
    ensureStructure();
    resetToDefault();
    // On the state handle itself: replaceState() assigns to it, which is what sends valueTreeRedirected.
    apvts_.state.addListener(this);
    rebuild();
}

PatternStore::~PatternStore() { apvts_.state.removeListener(this); }

juce::ValueTree PatternStore::root() { return apvts_.state.getChildWithName(idPatterns); }

void PatternStore::ensureStructure() {
    const bool was = inRebuild_;
    inRebuild_ = true;
    auto r = apvts_.state.getOrCreateChildWithName(idPatterns, nullptr);
    while (r.getNumChildren() < kPatterns) r.appendChild(juce::ValueTree(idPattern), nullptr);
    for (int p = 0; p < kPatterns; ++p) {
        auto pn = r.getChild(p);
        while (pn.getNumChildren() < kTracks) {
            juce::ValueTree t(idTrack);
            t.setProperty(idLen, 16, nullptr); t.setProperty(idRate, kDefaultRate, nullptr);
            pn.appendChild(t, nullptr);
        }
    }
    apvts_.state.getOrCreateChildWithName(idNames, nullptr);
    inRebuild_ = was;
}

juce::ValueTree PatternStore::trackNode(int pattern, int t) {
    return root().getChild(juce::jlimit(0, kPatterns - 1, pattern)).getChild(juce::jlimit(0, kTracks - 1, t));
}

void PatternStore::writeTrack(juce::ValueTree node, const TrackPattern& tp, juce::UndoManager* um) {
    if ((int)node.getProperty(idLen, 16) != tp.length) node.setProperty(idLen, juce::jlimit(1, kMaxSteps, tp.length), um);
    if ((int)node.getProperty(idRate, kDefaultRate) != tp.rate) node.setProperty(idRate, juce::jlimit(0, kNumRates - 1, tp.rate), um);
    for (int l = 0; l < NumLanes; ++l) {
        const juce::String s = laneToString(tp, (Lane)l);
        const juce::Identifier key(laneKey((Lane)l));
        if (node.getProperty(key).toString() != s) {
            if (s.isEmpty()) node.removeProperty(key, um);
            else node.setProperty(key, s, um);
        }
    }
}

int PatternStore::lane(int pattern, int t, int step, Lane l) const {
    return getStep(cache_.patterns[(size_t)pattern].tracks[(size_t)t].steps[(size_t)juce::jlimit(0, kMaxSteps - 1, step)], l);
}

void PatternStore::setLane(int pattern, int t, int step, Lane l, int value) {
    TrackPattern tp = track(pattern, t);
    setStep(tp.steps[(size_t)juce::jlimit(0, kMaxSteps - 1, step)], l, value);
    writeTrack(trackNode(pattern, t), tp, undo_);
}
void PatternStore::setTrack(int pattern, int t, const TrackPattern& tp) { writeTrack(trackNode(pattern, t), tp, undo_); }
void PatternStore::setLength(int pattern, int t, int length) { TrackPattern tp = track(pattern, t); tp.length = juce::jlimit(1, kMaxSteps, length); setTrack(pattern, t, tp); }
void PatternStore::setRate(int pattern, int t, int rate) { TrackPattern tp = track(pattern, t); tp.rate = juce::jlimit(0, kNumRates - 1, rate); setTrack(pattern, t, tp); }
void PatternStore::clearTrack(int pattern, int t) {
    TrackPattern tp = track(pattern, t);
    for (auto& s : tp.steps) s = Step{};
    setTrack(pattern, t, tp);
}
void PatternStore::clearPattern(int pattern) { Batch b(*this); for (int t = 0; t < kTracks; ++t) clearTrack(pattern, t); }
void PatternStore::copyPattern(int from, int to) {
    if (from == to) return;
    const Pattern src = cache_.patterns[(size_t)juce::jlimit(0, kPatterns - 1, from)];
    Batch b(*this);
    for (int t = 0; t < kTracks; ++t) setTrack(to, t, src.tracks[(size_t)t]);
}
void PatternStore::rotate(int pattern, int t, int by) {
    TrackPattern tp = track(pattern, t);
    const int len = juce::jlimit(1, kMaxSteps, tp.length);
    TrackPattern out = tp;
    for (int s = 0; s < len; ++s) out.steps[(size_t)(((s + by) % len + len) % len)] = tp.steps[(size_t)s];
    setTrack(pattern, t, out);
}

juce::String PatternStore::trackName(int t) const {
    const auto& n = names_[(size_t)juce::jlimit(0, kTracks - 1, t)];
    return n.isEmpty() ? defaultTrackName(t) : n;
}
void PatternStore::setTrackName(int t, const juce::String& name) {
    auto names = apvts_.state.getOrCreateChildWithName(idNames, nullptr);
    names.setProperty(juce::Identifier("n" + juce::String(juce::jlimit(0, kTracks - 1, t))), name.trim().substring(0, 24), undo_);
}

juce::var PatternStore::patternsToVar() const {
    juce::Array<juce::var> pats;
    for (int p = 0; p < kPatterns; ++p) {
        juce::Array<juce::var> tracks;
        for (int t = 0; t < kTracks; ++t) {
            const auto& tp = cache_.patterns[(size_t)p].tracks[(size_t)t];
            auto* o = new juce::DynamicObject();
            o->setProperty(idLen, tp.length); o->setProperty(idRate, kRateNames[juce::jlimit(0, kNumRates - 1, tp.rate)]);
            for (int l = 0; l < NumLanes; ++l) {
                const auto s = laneToString(tp, (Lane)l);
                if (s.isNotEmpty()) o->setProperty(laneKey((Lane)l), s);
            }
            tracks.add(juce::var(o));
        }
        pats.add(juce::var(tracks));
    }
    return pats;
}

void PatternStore::patternsFromVar(const juce::var& v) {
    ensureStructure();
    Batch b(*this);
    for (int p = 0; p < kPatterns; ++p) {
        const juce::var pv = v.isArray() && p < v.size() ? v[p] : juce::var();
        for (int t = 0; t < kTracks; ++t) {
            TrackPattern tp;
            const juce::var tv = pv.isArray() && t < pv.size() ? pv[t] : juce::var();
            if (tv.isObject()) {
                tp.length = juce::jlimit(1, kMaxSteps, (int)tv.getProperty(idLen, 16));
                const juce::var rate = tv.getProperty(idRate, kDefaultRate);
                if (rate.isString()) {
                    tp.rate = kDefaultRate;
                    for (int i = 0; i < kNumRates; ++i) if (rate.toString() == kRateNames[i]) tp.rate = i;
                } else tp.rate = juce::jlimit(0, kNumRates - 1, (int)rate);
                for (int l = 0; l < NumLanes; ++l) laneFromString(tp, (Lane)l, tv.getProperty(laneKey((Lane)l), "").toString());
            }
            writeTrack(trackNode(p, t), tp, undo_);
        }
    }
}

juce::var PatternStore::namesToVar() const {
    juce::Array<juce::var> a;
    for (int t = 0; t < kTracks; ++t) a.add(trackName(t));
    return a;
}
void PatternStore::namesFromVar(const juce::var& v) {
    Batch b(*this);
    for (int t = 0; t < kTracks; ++t) setTrackName(t, v.isArray() && t < v.size() ? v[t].toString() : defaultTrackName(t));
}

void PatternStore::resetToDefault() {
    // The Init beat: four on the floor, a backbeat clap, eighth hats with an open hat on the off-beat.
    juce::ScopedValueSetter<juce::UndoManager*> noUndo(undo_, nullptr);
    juce::Array<juce::var> pat, tracks;
    auto lanes = [](const char* v) { auto* o = new juce::DynamicObject(); o->setProperty("len", 16); o->setProperty("v", v); return juce::var(o); };
    tracks.add(lanes("110,0,0,0,110,0,0,0,110,0,0,0,110"));
    tracks.add(lanes(""));
    tracks.add(lanes("0,0,0,0,105,0,0,0,0,0,0,0,105"));
    tracks.add(lanes("80,0,70,0,80,0,70,0,80,0,70,0,80,0,70"));
    tracks.add(lanes("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85"));
    for (int t = 5; t < kTracks; ++t) tracks.add(lanes(""));
    pat.add(juce::var(tracks));
    patternsFromVar(pat);
    namesFromVar({});
}

bool PatternStore::ours(const juce::ValueTree& t) const {
    const auto type = t.getType();
    return type == idTrack || type == idPattern || type == idPatterns || type == idNames;
}

void PatternStore::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier&) { if (!inRebuild_ && ours(tree)) rebuild(); }
void PatternStore::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) { if (!inRebuild_ && (ours(parent) || ours(child))) rebuild(); }
void PatternStore::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int) { if (!inRebuild_ && (ours(parent) || ours(child))) rebuild(); }
void PatternStore::valueTreeRedirected(juce::ValueTree&) {
    // The state was replaced (host recall, A/B): fill in anything an older state lacks, then parse it.
    ensureStructure();
    rebuild();
}

void PatternStore::rebuild() {
    std::lock_guard<std::mutex> lock(writer_);
    inRebuild_ = true;
    auto r = apvts_.state.getChildWithName(idPatterns);
    for (int p = 0; p < kPatterns; ++p)
        for (int t = 0; t < kTracks; ++t) {
            TrackPattern tp;
            auto node = r.getChild(p).getChild(t);
            if (node.isValid()) {
                tp.length = juce::jlimit(1, kMaxSteps, (int)node.getProperty(idLen, 16));
                tp.rate = juce::jlimit(0, kNumRates - 1, (int)node.getProperty(idRate, kDefaultRate));
                for (int l = 0; l < NumLanes; ++l) laneFromString(tp, (Lane)l, node.getProperty(laneKey((Lane)l)).toString());
            }
            cache_.patterns[(size_t)p].tracks[(size_t)t] = tp;
        }
    auto names = apvts_.state.getChildWithName(idNames);
    for (int t = 0; t < kTracks; ++t) names_[(size_t)t] = names.getProperty(juce::Identifier("n" + juce::String(t))).toString();
    (*buffers_)[(size_t)back_] = cache_;
    back_ = mid_.exchange(back_ | 4, std::memory_order_acq_rel) & 3;
    inRebuild_ = false;
    ++version;
}

const PatternBank* PatternStore::acquireForAudio() {
    if (mid_.load(std::memory_order_acquire) & 4) front_ = mid_.exchange(front_, std::memory_order_acq_rel) & 3;
    return &(*buffers_)[(size_t)front_];
}
