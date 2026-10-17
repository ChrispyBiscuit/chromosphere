#pragma once
// Minimal WAV reader/writer for the command-line tools and tests (PCM 16/24/32 and float32 in,
// float32 out). The plugin loads user files with JUCE's format readers instead.
#include <string>
#include <vector>

namespace fr {
struct WavData { int rate = 48000; std::vector<std::vector<float>> ch; };
bool readWav(const std::string& path, WavData& out);
bool writeWav(const std::string& path, const WavData& in);   // any channel count, float32
}
