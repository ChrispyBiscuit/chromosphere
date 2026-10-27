// The update path end to end against a local "release": manifest parsing, version compare, download, checksum
// verification and rejection of a tampered file. No network: INFRA_UPDATE_MANIFEST points at file:// URLs.
#include "Updater.h"
#include <juce_cryptography/juce_cryptography.h>
#include <cstdio>
#include <cstdlib>
#include <thread>

static int failures = 0;
#define CHECK(c, ...) do { if (c) std::printf("PASS  "); else { ++failures; std::printf("FAIL  "); } std::printf(__VA_ARGS__); std::printf("\n"); } while (0)

static bool waitFor(Updater& u, std::initializer_list<Updater::State> states, double seconds = 20.0) {
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    while (juce::Time::getMillisecondCounterHiRes() - t0 < seconds * 1000.0) {
        for (auto s : states) if (u.state() == s) return true;
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    }
    return false;
}

static void setEnv(const char* k, const juce::String& v) {
#if defined(_WIN32)
    _putenv_s(k, v.toRawUTF8());
#else
    setenv(k, v.toRawUTF8(), 1);
#endif
}

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("infra_updater_test");
    dir.deleteRecursively(); dir.createDirectory();
    auto downloads = dir.getChildFile("downloads"); downloads.createDirectory();
    auto setup = dir.getChildFile("ChromosphereInfra-Setup.exe");
    juce::MemoryBlock payload; { juce::Random r(5); for (int i = 0; i < 300000; ++i) { const char b = (char)r.nextInt(256); payload.append(&b, 1); } }
    setup.replaceWithData(payload.getData(), payload.getSize());
    const auto sha = juce::SHA256(setup).toHexString();
    auto manifest = dir.getChildFile("latest.json");
    auto write = [&](const juce::String& version, const juce::String& hash) {
        juce::String json = "{ \"version\": \"" + version + "\", \"notes\": \"test\", \"downloads\": { \"windows\": \"URL\", \"mac\": \"URL\", \"linux\": \"URL\" },"
                            " \"sha256\": { \"windows\": \"" + hash + "\", \"mac\": \"" + hash + "\", \"linux\": \"" + hash + "\" } }";
        manifest.replaceWithText(json.replace("URL", juce::URL(setup).toString(false)));
    };
    setEnv("INFRA_UPDATE_MANIFEST", juce::URL(manifest).toString(false));
    setEnv("INFRA_UPDATE_DOWNLOAD_DIR", downloads.getFullPathName());

    Updater u;
    u.setAutomatic(false);
    // Same version as installed: up to date.
    write(Updater::currentVersion(), sha);
    u.checkNow();
    CHECK(waitFor(u, {Updater::State::UpToDate, Updater::State::Failed}) && u.state() == Updater::State::UpToDate, "same version reads as up to date (%s)", u.message().toRawUTF8());
    // A newer version is offered.
    write("99.0.0", sha);
    u.checkNow();
    CHECK(waitFor(u, {Updater::State::Available, Updater::State::Failed}) && u.state() == Updater::State::Available && u.info().newer, "99.0.0 is offered (%s)", u.message().toRawUTF8());
    CHECK(u.info().url.isNotEmpty(), "manifest has a download for this platform");
    // Download and verify.
    u.download();
    CHECK(waitFor(u, {Updater::State::Downloaded, Updater::State::Failed}) && u.state() == Updater::State::Downloaded, "download verified (%s)", u.message().toRawUTF8());
    const auto got = downloads.getChildFile("ChromosphereInfra-Setup-99.0.0.exe");   // named after its version
    CHECK(got.existsAsFile() && got.getSize() == (juce::int64)payload.getSize(), "installer saved to the download folder");
    // A wrong checksum is rejected and the file removed.
    got.deleteFile();
    const auto tampered = downloads.getChildFile("ChromosphereInfra-Setup-99.0.1.exe");
    write("99.0.1", juce::String::repeatedString("0", 64));
    u.checkNow();
    waitFor(u, {Updater::State::Available, Updater::State::Failed});
    u.download();
    CHECK(waitFor(u, {Updater::State::Downloaded, Updater::State::Failed}) && u.state() == Updater::State::Failed && !tampered.existsAsFile(),
          "tampered download rejected (%s)", u.message().toRawUTF8());
    // Skipping a version hides it.
    u.skipVersion("99.0.1");
    u.checkNow();
    CHECK(waitFor(u, {Updater::State::UpToDate, Updater::State::Available, Updater::State::Failed}) && u.state() == Updater::State::UpToDate, "skipped version not offered again");
    u.skipVersion({});
    dir.deleteRecursively();
    std::printf("%d failure(s)\n", failures);
    return failures;
}
