#pragma once
// Opt-in update check. Off by default; nothing touches the network until the user turns it on.
// Reads a small JSON manifest published with each release, compares versions, and offers to
// download the installer into the Downloads folder and launch it. The plug-in never replaces
// its own files. Message thread API; the network work runs on its own thread.
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class Updater : private juce::Thread, private juce::AsyncUpdater, private juce::Timer {
public:
    struct Info { juce::String version, notes, url, sha256; bool newer = false; };
    enum class State { Idle, Checking, UpToDate, Available, Downloading, Downloaded, Scheduled, Failed };

    Updater();
    ~Updater() override;

    bool enabled();
    void setEnabled(bool on);
    // Automatic mode (Windows): download new versions without asking and install them when the DAW closes.
    bool automatic();
    void setAutomatic(bool on);
    static bool automaticSupported();
    // Graphics preference (stored with the other settings): GPU (OpenGL) rendering of the sun. Off by
    // default: OpenGL inside some hosts has hung whole systems; the CPU renderer is the safe path.
    bool gpuGraphics();
    void setGpuGraphics(bool on);
    juce::String skippedVersion();
    void skipVersion(const juce::String& v);

    void checkNow();                         // explicit, regardless of the preference
    void checkIfDue();                       // at most once per 24 h, only when enabled
    void download();                         // fetch the installer for the available version
    void openDownloaded();                   // launch what download() fetched (the installer asks to close the DAW)
    // Launch the downloaded installer in waiting mode: it installs silently once this process has exited.
    bool scheduleInstall();
    juce::String pendingVersion();          // version an installer is waiting to apply, or empty

    State state() const { return state_.load(); }
    Info info();
    juce::String message();
    float progress() const { return progress_.load(); }
    std::function<void()> onChange;          // called on the message thread after any change

    static juce::String currentVersion();
    static juce::String manifestUrl();

private:
    void run() override;
    void handleAsyncUpdate() override;
    void timerCallback() override;               // one shot after construction: the daily (or hourly) check
    void reconcilePending();                     // drop a pending install that a previous session failed to apply
    bool launchWaiter(const juce::File& installer);
    void set(State s, const juce::String& msg);
    static int compareVersions(const juce::String& a, const juce::String& b);
    juce::PropertiesFile& props();

    std::unique_ptr<juce::PropertiesFile> props_;
    std::atomic<State> state_{State::Idle};
    std::atomic<float> progress_{0.f};
    Info info_;
    juce::String message_;
    juce::File downloaded_;
    bool wantDownload_ = false;
    std::atomic<bool> wantSchedule_{false};      // set by the download thread, acted on by the message thread
    juce::CriticalSection lock_;
};
