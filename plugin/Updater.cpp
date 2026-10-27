#include "Updater.h"
#include <juce_cryptography/juce_cryptography.h>
#if JUCE_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {
const char* kRepoOwner = "ChrispyBiscuit";
const char* kRepoName = "Chromosphere";
}

// The check also runs without the editor open: a few seconds after the plug-in loads.
Updater::Updater() : juce::Thread("Chromosphere updater") { startTimer(6000); }
Updater::~Updater() { stopTimer(); cancelPendingUpdate(); stopThread(5000); }

void Updater::timerCallback() { stopTimer(); checkIfDue(); }

void Updater::handleAsyncUpdate() {
    if (wantSchedule_.exchange(false)) scheduleInstall();
    if (onChange) onChange();
}

namespace {
juce::int64 thisProcessId() {
#if JUCE_WINDOWS
    return (juce::int64)GetCurrentProcessId();
#else
    return (juce::int64)getpid();
#endif
}
}

// A pending install was handed to a waiter in an earlier session. If that session has ended (a
// different process now) and this build is still older than the pending version, the install did
// not happen (UAC declined, host killed the waiter): forget it so the next check tries again.
void Updater::reconcilePending() {
    const auto pend = pendingVersion();
    if (pend.isEmpty()) return;
    const auto pid = props().getValue("updates.pendingPid").getLargeIntValue();
    if (compareVersions(currentVersion(), pend) >= 0 || pid != thisProcessId()) {
        props().removeValue("updates.pending"); props().removeValue("updates.pendingPid"); props().saveIfNeeded();
    }
}

juce::PropertiesFile& Updater::props() {
    if (!props_) {
        juce::PropertiesFile::Options o;
        o.applicationName = "Chromosphere"; o.filenameSuffix = "settings"; o.folderName = "Chromosphere";
        o.osxLibrarySubFolder = "Application Support"; o.storageFormat = juce::PropertiesFile::storeAsXML;
        props_ = std::make_unique<juce::PropertiesFile>(o);
    }
    return *props_;
}

bool Updater::enabled() { return props().getBoolValue("updates.enabled", false); }
bool Updater::automatic() { return automaticSupported() && props().getBoolValue("updates.auto", false); }
void Updater::setAutomatic(bool on) { props().setValue("updates.auto", on); props().saveIfNeeded(); triggerAsyncUpdate(); }
bool Updater::gpuGraphics() { return props().getBoolValue("graphics.opengl", false); }
void Updater::setGpuGraphics(bool on) { props().setValue("graphics.opengl", on); props().saveIfNeeded(); }
bool Updater::automaticSupported() {
#if JUCE_WINDOWS
    return true;
#else
    return false;
#endif
}
juce::String Updater::pendingVersion() { return props().getValue("updates.pending"); }

bool Updater::scheduleInstall() {
    if (!downloaded_.existsAsFile() || !automaticSupported()) return false;
    if (pendingVersion() == info().version && props().getValue("updates.pendingPid").getLargeIntValue() == thisProcessId()) return true;   // already handed over
    if (!launchWaiter(downloaded_)) { set(State::Failed, "Could not start the installer"); return false; }
    props().setValue("updates.pending", info().version);
    props().setValue("updates.pendingPid", juce::String(thisProcessId()));
    props().saveIfNeeded();
    set(State::Scheduled, "Version " + info().version + " installs when you close your DAW");
    return true;
}

// The installer needs administrator rights (it writes to Program Files). Launching it from the DAW
// would show the Windows permission prompt at once, often behind the DAW window, and if it is
// missed or declined nothing installs. Instead a hidden, unprivileged PowerShell waits for this
// process to exit and only then starts the installer with elevation, so the prompt appears on a
// clear desktop right after the DAW closes. The waiter breaks away from the host's job object
// where allowed, so closing the DAW does not take it down too.
bool Updater::launchWaiter(const juce::File& installer) {
#if JUCE_WINDOWS
    const juce::String path = installer.getFullPathName().replace("'", "''");
    const juce::String script =
        "try { Wait-Process -Id " + juce::String(thisProcessId()) + " -ErrorAction Stop } catch {}; "
        "Start-Sleep -Seconds 2; "
        "Start-Process -FilePath '" + path + "' -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -Verb RunAs";
    juce::String cmd = "powershell.exe -NoProfile -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -Command \"" + script + "\"";
    STARTUPINFOW si{}; si.cb = sizeof(si); si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    auto attempt = [&](DWORD flags) {
        std::wstring buf(cmd.toWideCharPointer());   // CreateProcessW may write to the command line
        return CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi) != 0;
    };
    const DWORD base = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
    if (!attempt(base | CREATE_BREAKAWAY_FROM_JOB) && !attempt(base)) return false;
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return true;
#else
    juce::ignoreUnused(installer);
    return false;
#endif
}
void Updater::setEnabled(bool on) { props().setValue("updates.enabled", on); props().saveIfNeeded(); triggerAsyncUpdate(); }
juce::String Updater::skippedVersion() { return props().getValue("updates.skip"); }
void Updater::skipVersion(const juce::String& v) { props().setValue("updates.skip", v); props().saveIfNeeded(); set(State::UpToDate, "Version " + v + " skipped"); }
Updater::Info Updater::info() { const juce::ScopedLock sl(lock_); return info_; }
juce::String Updater::message() { const juce::ScopedLock sl(lock_); return message_; }

juce::String Updater::currentVersion() { return JucePlugin_VersionString; }
juce::String Updater::manifestUrl() {
    return "https://github.com/" + juce::String(kRepoOwner) + "/" + kRepoName + "/releases/download/nightly/latest.json";
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
    reconcilePending();
    const auto last = props().getValue("updates.lastCheck").getLargeIntValue();
    const juce::int64 interval = automatic() ? 3600LL * 1000 : 24LL * 3600 * 1000;   // automatic mode keeps closer to the latest build
    if (juce::Time::currentTimeMillis() - last < interval) return;
    checkNow();
}

void Updater::checkNow() {
    if (isThreadRunning()) return;
    reconcilePending();
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
    if (!wantDownload_) {
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
        if (!in.newer) { props().removeValue("updates.pending"); set(State::UpToDate, "Up to date (version " + currentVersion() + ")"); }
        else if (in.version == pendingVersion()) set(State::Scheduled, "Version " + in.version + " installs when you close your DAW");
        else if (in.version == skippedVersion()) set(State::UpToDate, "Version " + in.version + " is available (skipped)");
        else if (in.url.isEmpty()) set(State::Available, "Version " + in.version + " is available, with no download for this platform");
        else if (automatic()) { wantDownload_ = true; set(State::Downloading, "Downloading " + in.version); }   // falls through to the download below
        else { set(State::Available, "Version " + in.version + " is available"); return; }
        if (!wantDownload_) return;
    }
    // Download the installer to the Downloads folder, then verify the checksum.
    const Info in = info();
    juce::URL url(in.url);
    const auto name = url.getFileName().isEmpty() ? juce::String("Chromosphere-Setup") : url.getFileName();
    // Versioned name, so an installer for one build is never mistaken for another.
    const auto versioned = name.upToLastOccurrenceOf(".", false, false) + "-" + in.version + "." + name.fromLastOccurrenceOf(".", false, false);
    auto dest = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads").getChildFile(versioned);
    if (!dest.getParentDirectory().isDirectory()) dest = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(versioned);
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
    if (automatic()) {
        // Hand over to the message thread (scheduleInstall touches preferences and state); the
        // AsyncUpdater is cancelled in the destructor, so this never outlives the updater.
        wantSchedule_ = true;
        set(State::Downloaded, "Downloaded " + in.version);
        return;
    }
    set(State::Downloaded, "Saved to " + dest.getFullPathName() + ". Close your DAW, then run it.");
}
