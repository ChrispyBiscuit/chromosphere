#pragma once
// Update check for ULTRA (ported from EXO via INFRA). The daily check is on by default and can be turned off; it reads
// one small JSON manifest from the release page and nothing else. A newer version can be downloaded (checksum
// verified) and installed with one click, or, in automatic mode (Windows), downloaded and installed silently
// once the host (DAW or the standalone app) has closed. The plug-in never replaces its own files: the
// installer does. Message thread API; the network work runs on its own thread.
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class Updater : private juce::Thread, private juce::AsyncUpdater {
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
    juce::String skippedVersion();
    void skipVersion(const juce::String& v);

    void checkNow();                         // explicit, regardless of the preference
    void checkIfDue();                       // at most once per 24 h, only when enabled
    void download();                         // fetch the installer for the available version
    void openDownloaded();                   // launch what download() fetched (the installer asks to close the DAW)
    void installNow();                       // download if needed, then install when the host closes (Windows) or open the installer
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
    static juce::String hostName();          // "your DAW" or "the app" (standalone)

private:
    void run() override;
    void handleAsyncUpdate() override { if (onChange) onChange(); }
    void set(State s, const juce::String& msg);
    static int compareVersions(const juce::String& a, const juce::String& b);
    juce::PropertiesFile& props();

    std::unique_ptr<juce::PropertiesFile> props_;
    std::atomic<State> state_{State::Idle};
    std::atomic<float> progress_{0.f};
    Info info_;
    juce::String message_;
    juce::File downloaded_;
    bool wantDownload_ = false, installAfterDownload_ = false;
    juce::CriticalSection lock_;
};
