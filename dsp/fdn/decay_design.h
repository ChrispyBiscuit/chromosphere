#pragma once
#include <array>
#include <vector>

namespace fr {

constexpr int kShelves = 4;

struct Shelf { double b0, b1, a1; };  // y = b0 x + s;  s' = b1 x - a1 y

struct DecayFilter {
    double gain;                       // broadband per-pass gain (linear)
    std::array<Shelf, kShelves> sh;    // cascade of first-order shelves
};

// T60 (seconds) at f Hz: decay rate (dB/s) interpolated in log2(f) between the
// anchors 125 Hz, 1 kHz, 8 kHz; flat outside.
double t60Target(double f, const double t60[3]);

// Fit gain + shelves so a line of `delaySamples` loses the right level per pass
// to give the target T60 curve on a half-octave grid, 125 Hz .. 16 kHz.
DecayFilter designDecayFilter(double delaySamples, const double t60[3], double fs);

// Magnitude (dB) of a decay filter at f Hz (for tests and tooling).
double decayFilterDb(const DecayFilter& d, double f, double fs);

}  // namespace fr
