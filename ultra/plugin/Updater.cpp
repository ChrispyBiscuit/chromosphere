#include "Updater.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_cryptography/juce_cryptography.h>
#if JUCE_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

// The repository whose "ultra-nightly" release CI publishes to (set by CMake from ULTRA_UPDATE_REPO).
#ifndef ULTRA_UPDATE_REPO
#define ULTRA_UPDATE_REPO "ChrispyBiscuit/chromosphere"
#endif

namespace {
juce::int64 processId() {
#if JUCE_WINDOWS
    return (juce::int64)GetCurrentProcessId();
#else
    return (juce::int64)getpid();
#endif
}
}  // namespace

Updater::Updater() : juce::Thread("Chromosphere ULTRA updater") {
    // An install waits for the host that scheduled it to exit. If this is a later process and the version did not
    // change, that install did not happen (declined, failed): forget it so the next check offers it again.
    auto& p = props();
    p.reload();
    if (p.getValue("updates.pending").isNotEmpty() && p.getValue("updates.pendingPid").getLargeIntValue() != processId()) {
        p.removeValue("updates.pending"); p.removeValue("updates.pendingPid");
        p.saveIfNeeded();
    }
}
Updater::~Updater() { stopTimer(); stopThread(5000); }

void Updater::scheduleFirstCheck() {
    if (firstCheckScheduled_) return;
    firstCheckScheduled_ = true;
    startTimer(6000);           // well after the session has loaded
}

juce::PropertiesFile& Updater::props() {
    if (!props_) {
        juce::PropertiesFile::Options o;
        o.applicationName = "ChromosphereUltra"; o.filenameSuffix = "settings"; o.folderName = "ChromosphereUltra";
        o.osxLibrarySubFolder = "Application Support"; o.storageFormat = juce::PropertiesFile::storeAsXML;
        o.processLock = &settingsLock_;                    // held only while the file loads or saves
        o.millisecondsBeforeSaving = -1;                     // written only by saveIfNeeded(), after a real change
        props_ = std::make_unique<juce::PropertiesFile>(o);
    }
    return *props_;
}

bool Updater::enabled() { props().reload(); return props().getBoolValue("updates.enabled", true); }
bool Updater::automatic() { props().reload(); return automaticSupported() && props().getBoolValue("updates.auto", false); }
void Updater::setAutomatic(bool on) { props().reload(); props().setValue("updates.auto", on); props().saveIfNeeded(); triggerAsyncUpdate(); }
bool Updater::automaticSupported() {
#if JUCE_WINDOWS
    return true;
#else
    return false;
#endif
}
juce::String Updater::pendingVersion() { props().reload(); return props().getValue("updates.pending"); }

// The installer needs administrator rights. Started from the DAW, its prompt would open behind the DAW window and be
// missed. Instead a hidden, unprivileged waiter (PowerShell, no window, its own process group, out of the DAW's job)
// waits for this process to exit, then starts the installer silently with the elevation prompt (EXO's guide, 6).
bool Updater::startWaiter(const juce::File& installer, bool relaunch) {
#if JUCE_WINDOWS
    auto quote = [](const juce::String& v) { return "'" + v.replace("'", "''") + "'"; };
    juce::String args = "'/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'";
    if (relaunch) args << ",'/RELAUNCH=1'";
    const juce::String script = "try { Wait-Process -Id " + juce::String(processId()) + " -ErrorAction SilentlyContinue } catch {}; "
                                "Start-Sleep -Seconds 2; Start-Process -FilePath " + quote(installer.getFullPathName()) +
                                " -ArgumentList " + args + " -Verb RunAs";
    juce::String cmd = "powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -Command \"" + script + "\"";
    STARTUPINFOW si{}; si.cb = sizeof(si); si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::wstring line = cmd.toWideCharPointer();
    const DWORD base = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
    BOOL ok = CreateProcessW(nullptr, line.data(), nullptr, nullptr, FALSE, base | CREATE_BREAKAWAY_FROM_JOB, nullptr, nullptr, &si, &pi);
    if (!ok) { line = cmd.toWideCharPointer(); ok = CreateProcessW(nullptr, line.data(), nullptr, nullptr, FALSE, base, nullptr, nullptr, &si, &pi); }   // the job may forbid breakaway
    if (!ok) return false;
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return true;
#else
    juce::ignoreUnused(installer, relaunch);
    return false;
#endif
}

bool Updater::scheduleInstall() {
    if (!downloaded_.existsAsFile() || !automaticSupported()) return false;
    const bool relaunch = juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone;   // reopen the app afterwards
    if (!startWaiter(downloaded_, relaunch)) { set(State::Failed, "Could not start the installer"); return false; }
    props().reload();
    props().setValue("updates.pending", info().version); props().setValue("updates.pendingPid", juce::String(processId()));
    props().saveIfNeeded();
    set(State::Scheduled, "Version " + info().version + " installs when you close " + hostName());
    return true;
}

void Updater::installNow() {
    if (state_ == State::Downloaded) {
        if (automaticSupported()) scheduleInstall(); else openDownloaded();
        return;
    }
    installAfterDownload_ = true;
    download();
}
void Updater::setEnabled(bool on) { props().reload(); props().setValue("updates.enabled", on); props().saveIfNeeded(); triggerAsyncUpdate(); }
juce::String Updater::skippedVersion() { props().reload(); return props().getValue("updates.skip"); }
void Updater::skipVersion(const juce::String& v) { props().reload(); props().setValue("updates.skip", v); props().saveIfNeeded(); set(State::UpToDate, "Version " + v + " skipped"); }
juce::String Updater::hostName() {
    return juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone ? "the app" : "your DAW";
}

Updater::Info Updater::info() { const juce::ScopedLock sl(lock_); return info_; }
juce::String Updater::message() { const juce::ScopedLock sl(lock_); return message_; }

juce::String Updater::currentVersion() { return JucePlugin_VersionString; }
juce::String Updater::manifestUrl() {
    // ULTRA_UPDATE_MANIFEST overrides the release URL (tests, or pointing a build at a staging release).
    const auto env = juce::SystemStats::getEnvironmentVariable("ULTRA_UPDATE_MANIFEST", {});
    if (env.isNotEmpty()) return env;
    return "https://github.com/" + juce::String(ULTRA_UPDATE_REPO) + "/releases/download/ultra-nightly/latest.json";
}

int Updater::compareVersions(const juce::String& a, const juce::String& b) {
    juce::StringArray pa, pb; pa.addTokens(a, ".", ""); pb.addTokens(b, ".", "");
    for (int i = 0; i < std::max(pa.size(), pb.size()); ++i) {
        const int x = i < pa.size() ? pa[i].getIntValue() : 0, y = i < pb.size() ? pb[i].getIntValue() : 0;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

void Updater::set(State s, const juce::String& msg) {
    { const juce::ScopedLock sl(lock_); message_ = msg; }
    state_ = s;
    triggerAsyncUpdate();
}

void Updater::checkIfDue() {
    if (!enabled() || isThreadRunning()) return;
    if (juce::SystemStats::getEnvironmentVariable("ULTRA_NO_UPDATE_CHECK", {}).isNotEmpty()) return;   // tests, snapshots
    props().reload();                         // another process may have checked since
    const auto last = props().getValue("updates.lastCheck").getLargeIntValue();
    if (juce::Time::currentTimeMillis() - last < 24LL * 3600 * 1000) return;
    checkNow();
}

void Updater::checkNow() {
    if (isThreadRunning()) return;
    wantDownload_ = false;
    set(State::Checking, "Checking for updates");
    startThread();
}

void Updater::download() {
    if (isThreadRunning() || state_ != State::Available) return;
    wantDownload_ = true;
    progress_ = 0.f;
    set(State::Downloading, "Downloading " + info().version);
    startThread();
}

void Updater::openDownloaded() { if (downloaded_.existsAsFile()) downloaded_.startAsProcess(); }

void Updater::run() {
    // One process at a time talks to the network: another DAW or scanner that is checking or downloading right now
    // will have written the result to the shared settings by the time this one looks again.
    juce::InterProcessLock net("ChromosphereUltraUpdaterNet");
    if (!net.enter(0)) { set(State::Idle, "Another copy of ULTRA is checking for updates"); return; }
    struct Exit { juce::InterProcessLock& l; ~Exit() { l.exit(); } } exitLock{net};
    if (!wantDownload_) {
        props().reload();
        props().setValue("updates.lastCheck", juce::String(juce::Time::currentTimeMillis())); props().saveIfNeeded();
        juce::URL url(manifestUrl());
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(8000));
        if (!stream) { set(State::Failed, "Could not reach the update server"); return; }
        const auto json = juce::JSON::parse(stream->readEntireStreamAsString());
        if (!json.isObject()) { set(State::Failed, "Update information was not readable"); return; }
        Info in;
        in.version = json["version"].toString();
        in.notes = json["notes"].toString();
#if JUCE_WINDOWS
        const juce::Identifier platform("windows");
#elif JUCE_MAC
        const juce::Identifier platform("mac");
#else
        const juce::Identifier platform("linux");
#endif
        in.url = json["downloads"][platform].toString();
        in.sha256 = json["sha256"][platform].toString();
        in.newer = compareVersions(currentVersion(), in.version) < 0;
        { const juce::ScopedLock sl(lock_); info_ = in; }
        if (!in.newer) { props().removeValue("updates.pending"); props().saveIfNeeded(); set(State::UpToDate, "Up to date (version " + currentVersion() + ")"); }
        else if (in.version == pendingVersion()) set(State::Scheduled, "Version " + in.version + " installs when you close " + hostName());
        else if (in.version == skippedVersion()) set(State::UpToDate, "Version " + in.version + " is available (skipped)");
        else if (in.url.isEmpty()) set(State::Available, "Version " + in.version + " is available, with no download for this platform");
        else if (automatic()) { wantDownload_ = true; set(State::Downloading, "Downloading " + in.version); }   // falls through to the download below
        else { set(State::Available, "Version " + in.version + " is available"); return; }
        if (!wantDownload_) return;
    }
    // Download the installer to the Downloads folder, then verify the checksum.
    const Info in = info();
    juce::URL url(in.url);
    const auto name = url.getFileName().isEmpty() ? juce::String("ChromosphereUltra-Setup.exe") : url.getFileName();
    const auto dirEnv = juce::SystemStats::getEnvironmentVariable("ULTRA_UPDATE_DOWNLOAD_DIR", {});   // tests
    auto dest = (dirEnv.isNotEmpty() ? juce::File(dirEnv) : juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads")).getChildFile(name);
    if (!dest.getParentDirectory().isDirectory()) dest = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(name);
    dest.deleteFile();
    auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(10000));
    if (!stream) { set(State::Failed, "The download did not start"); return; }
    {
        juce::FileOutputStream out(dest);
        if (!out.openedOk()) { set(State::Failed, "Cannot write to " + dest.getFullPathName()); return; }
        const juce::int64 total = stream->getTotalLength();
        juce::HeapBlock<char> buf(1 << 16);
        juce::int64 done = 0;
        while (!stream->isExhausted() && !threadShouldExit()) {
            const int n = stream->read(buf, 1 << 16);
            if (n <= 0) break;
            out.write(buf, (size_t)n);
            done += n;
            progress_ = total > 0 ? (float)done / (float)total : 0.f;
        }
        out.flush();
    }
    if (threadShouldExit()) { dest.deleteFile(); set(State::Failed, "Download cancelled"); return; }
    if (in.sha256.isNotEmpty() && !juce::SHA256(dest).toHexString().equalsIgnoreCase(in.sha256)) {
        dest.deleteFile(); set(State::Failed, "The downloaded file did not verify"); return;
    }
    downloaded_ = dest;
    if (automatic() || installAfterDownload_) {
        installAfterDownload_ = false;
        // Hand over to the message thread: scheduleInstall touches preferences and state.
        juce::MessageManager::callAsync([this] { if (automaticSupported()) scheduleInstall(); else openDownloaded(); });
        set(State::Downloaded, "Downloaded " + in.version);
        return;
    }
    set(State::Downloaded, "Saved to " + dest.getFullPathName() + ". Close " + hostName() + ", then run it.");
}
