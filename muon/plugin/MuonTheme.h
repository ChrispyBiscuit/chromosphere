#pragma once
// MUON's place in the shared Chromosphere look: Cherenkov blue (the light a muon gives off in a detector tank), a
// slightly cooler grey cast, a blue sun, and its embedded fonts and images. The VFD keeps its cyan and the
// readouts their red; the accent is a deeper, more saturated blue so the two still read as different instruments.
#include "Look.h"
#include <MuonData.h>

inline look::Theme muonTheme() {
    look::Theme t;
    t.wordmark = "CHROMOSPHERE | MUON";
    t.product = "Chromosphere MUON";
    t.bg = juce::Colour(0xff101214); t.plate = juce::Colour(0xff1a1b1e); t.plateEdge = juce::Colour(0xff26282c);
    t.line = juce::Colour(0xff2a2d31); t.text = juce::Colour(0xffe6ebf0); t.textDim = juce::Colour(0xff949ba3);
    t.textFaint = juce::Colour(0xff585e65); t.accent = juce::Colour(0xff2f8cff); t.accentDim = juce::Colour(0xff173f73);
    t.displayBg = juce::Colour(0xff0a0c0e); t.grid = juce::Colour(0xff1a1e22); t.curve = juce::Colour(0xff5fb0ff);
    t.curveFill = juce::Colour(0x402f8cff); t.meter = juce::Colour(0xffbcc6d0); t.meterHot = juce::Colour(0xff2f8cff);
    t.danger = juce::Colour(0xffff4f6a);
    auto& s = t.sun;
    s.deep = {0.02f, 0.06f, 0.30f}; s.mid = {0.10f, 0.42f, 1.0f}; s.hot = {0.72f, 0.88f, 1.0f}; s.corona = {0.15f, 0.42f, 1.0f};
    s.flare = {0.75f, 0.90f, 1.0f}; s.halo = {0.35f, 0.60f, 1.0f}; s.particle = {0.30f, 0.60f, 1.0f};
    s.radiation = {0.70f, 0.85f, 1.0f}; s.cme = {0.05f, 0.12f, 0.30f};
    s.clear = juce::Colour(0xff0a0c0e);
    t.resource = [](const char* name, int& size) -> const char* { return MuonData::getNamedResource(name, size); };
    return t;
}
