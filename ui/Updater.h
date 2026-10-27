#pragma once
// Update check shared by the Chromosphere products (from EXO; first used by INFRA). The daily check is on by
// default and can be turned off; it reads one small JSON manifest from the product's release page and nothing
// else. A newer version can be downloaded (checksum verified) and installed with one click, or, in automatic mode
// (Windows), downloaded and installed silently once the host (DAW or standalone app) has closed. The plug-in never
// replaces its own files: the installer does. Message thread API; the network work runs on its own thread.
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// What differs between products: where the release lives and what things are called.
struct UpdaterProduct {
    juce::String id = "ChromosphereInfra";            // settings folder and file, default installer name
    juce::String displayName = "Chromosphere INFRA";  // thread name
    juce::String repo = "ChrispyBiscuit/chromosphere";
    juce::String releaseTag = "infra-nightly";        // GitHub release holding latest.json and the installer
    juce::String envPrefix = "INFRA";                 // <prefix>_UPDATE_MANIFEST, _NO_UPDATE_CHECK, _UPDATE_DOWNLOAD_DIR
    juce::String installerName = "ChromosphereInfra-Setup.exe";
};

// One updater per process (optimization guide 4): all instances of a plug-in share it through SharedUpdater, editors
// listen with a ChangeListener, network checks, downloads and settings writes run under a cross-process lock, and the
// daily check starts a few seconds after it is first asked for, not while a session loads.
class Updater : public juce::ChangeBroadcaster, private juce::Thread, private juce::AsyncUpdater, private juce::Timer {
public:
    struct Info { juce::String version, notes, url, sha256; bool newer = false; };
    enum class State { Idle, Checking, UpToDate, Available, Downloading, Downloaded, Scheduled, Failed };

    explicit Updater(UpdaterProduct product);
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
    void checkIfDue();                       // at most once per 24 h, only when enabled; starts after a few seconds
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

    static juce::String currentVersion();
    juce::String manifestUrl() const;
    static juce::String hostName();          // "your DAW" or "the app" (standalone)

private:
    void run() override;
    void handleAsyncUpdate() override { sendChangeMessage(); }
    void timerCallback() override;
    bool lockForWork();                      // the cross-process lock, with a timeout
    void set(State s, const juce::String& msg);
    static int compareVersions(const juce::String& a, const juce::String& b);
    juce::PropertiesFile& props();

    UpdaterProduct product_;
    juce::InterProcessLock processLock_;
    std::unique_ptr<juce::PropertiesFile> props_;
    std::atomic<State> state_{State::Idle};
    std::atomic<float> progress_{0.f};
    Info info_;
    juce::String message_;
    juce::File downloaded_;
    bool wantDownload_ = false, installAfterDownload_ = false;
    juce::CriticalSection lock_;
};

// Defined once by each plug-in: its update channel.
UpdaterProduct updaterProductForThisPlugin();

// The per-process updater: juce::SharedResourcePointer<SharedUpdater> in each processor.
struct SharedUpdater {
    SharedUpdater() : updater(updaterProductForThisPlugin()) {}
    Updater updater;
};
