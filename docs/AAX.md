# Building for Pro Tools (AAX)

The code is set up for AAX but **has never been built or run as AAX**: the Avid AAX SDK cannot be redistributed and
needs a developer account. This file lists what you have to do. Nothing here is tested.

## What you need from Avid
1. A free Avid developer account (developer.avid.com) and the AAX SDK download. Unzip it somewhere, for example `C:\SDKs\AAX_SDK`.
2. To load the plugin in a normal Pro Tools you need it signed by PACE (Avid's partner) with the tools Avid gives developers
   (`wraptool`) and an iLok account for the signing key. Without signing, only a **Pro Tools Developer** build loads it.
3. The AAX Validator (part of the SDK, run with DigiShell) to check the plugin before you try it in a session.

## Build
    cmake -S . -B build -DFR_BUILD_PLUGIN=ON -DFR_AAX_SDK_PATH=C:/SDKs/AAX_SDK
    cmake --build build --config Release --target Chromosphere_AAX

The result is `build/Chromosphere_artefacts/Release/AAX/Chromosphere.aaxplugin`. Copy it to
`C:\Program Files\Common Files\Avid\Audio\Plug-Ins\`. Sign it with `wraptool` (see the SDK's documentation)
before you try a retail Pro Tools. Target: Pro Tools 2023 and newer, 44.1 to 192 kHz.

## Things to check in Pro Tools
- It reports 0 samples of latency, so delay compensation should show nothing.
- Pro Tools runs each plugin instance on one thread per channel. The audio path here uses one thread. The extra worker thread only builds
  engines and loads IRs; it never touches audio.
- Buffer sizes from 32 to 1024 samples, and sample-rate changes. The engine rebuilds when the rate changes.
- Save and reopen a session, and automate a few parameters (the IDs are stable, see `plugin/PluginProcessor.cpp`).
- Mono and stereo tracks (mono in, stereo out and stereo in, stereo out are supported; other layouts are refused).

## What JUCE does not give you
- **AudioSuite** (offline processing inside Pro Tools) is not supported by JUCE's AAX wrapper, so there is no AudioSuite version.
  The PRD item "AudioSuite offline render matches real time" cannot be met without extra work against the AAX SDK.
- **AAX DSP** (HDX hardware) is out of scope.
