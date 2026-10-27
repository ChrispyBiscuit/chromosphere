#pragma once
// The patterns live in the plugin's value tree (child "PATTERNS"), one compact string per lane, so host state,
// presets, A/B and undo cover them like any parameter. Every change is parsed into a plain ultra::PatternBank and
// published to the audio thread through a lock-free triple buffer (PRD Section 10). The audio thread never waits
// and never sees a half-written pattern.
#include "ultra/params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>

class PatternStore : private juce::ValueTree::Listener {
public:
    enum Lane { Vel, Chance, Ratchet, Pitch, Nudge, Cond, Lock, NumLanes };
    static const char* laneKey(Lane l);
    static int laneMin(Lane l);
    static int laneMax(Lane l);
    static int laneDefault(Lane l);

    PatternStore(juce::AudioProcessorValueTreeState& state, juce::UndoManager* undo);
    ~PatternStore() override;

    // Audio thread: the newest published bank. The pointer stays valid until the next acquire.
    const ultra::PatternBank* acquireForAudio();

    // Message thread.
    const ultra::PatternBank& bank() const { return cache_; }
    const ultra::TrackPattern& track(int pattern, int t) const { return cache_.patterns[(size_t)pattern].tracks[(size_t)t]; }
    int lane(int pattern, int t, int step, Lane l) const;
    void setLane(int pattern, int t, int step, Lane l, int value);
    void setTrack(int pattern, int t, const ultra::TrackPattern& tp);
    void setLength(int pattern, int t, int length);
    void setRate(int pattern, int t, int rate);
    void clearTrack(int pattern, int t);
    void clearPattern(int pattern);
    void copyPattern(int from, int to);
    void rotate(int pattern, int t, int by);
    juce::String trackName(int t) const;
    void setTrackName(int t, const juce::String& name);
    static juce::String defaultTrackName(int t);

    // Presets: the patterns and names as JSON-like vars (lanes as the same compact strings).
    juce::var patternsToVar() const;
    void patternsFromVar(const juce::var& v);      // missing patterns become empty
    juce::var namesToVar() const;
    void namesFromVar(const juce::var& v);
    void resetToDefault();                         // the Init beat
    juce::ValueTree root();                        // the PATTERNS node

    std::atomic<int> version{0};                   // bumps on every change (the editor repaints on it)

    static juce::String laneToString(const ultra::TrackPattern& tp, Lane l);
    static void laneFromString(ultra::TrackPattern& tp, Lane l, const juce::String& s);

private:
    // Many tree writes, one rebuild at the end.
    struct Batch {
        explicit Batch(PatternStore& s) : st(s), was(s.inRebuild_) { s.inRebuild_ = true; }
        ~Batch() { st.inRebuild_ = was; if (!was) st.rebuild(); }
        PatternStore& st; bool was;
    };
    juce::ValueTree trackNode(int pattern, int t);
    void writeTrack(juce::ValueTree node, const ultra::TrackPattern& tp, juce::UndoManager* um);
    void rebuild();
    void ensureStructure();
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree&) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree&, int) override;
    void valueTreeRedirected(juce::ValueTree&) override;
    bool ours(const juce::ValueTree& t) const;

    juce::AudioProcessorValueTreeState& apvts_;
    juce::UndoManager* undo_;
    ultra::PatternBank cache_{};
    std::array<juce::String, ultra::kTracks> names_;
    std::mutex writer_;
    bool inRebuild_ = false;

    // Triple buffer: the writer fills `back_`, then swaps it with `mid_` (flagged fresh); the reader swaps `front_`
    // with `mid_` when it is fresh.
    std::unique_ptr<std::array<ultra::PatternBank, 3>> buffers_;
    std::atomic<int> mid_{1};
    int front_ = 0, back_ = 2;
};
