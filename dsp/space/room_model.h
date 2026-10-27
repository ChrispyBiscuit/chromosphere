#pragma once
// Physical room model for the Acoustic engine (no JUCE, no audio-thread code).
//
// A shoebox room: dimensions, one material per surface group, a stereo source (the input's left and
// right channels as two sources across the stage), a listener with a two-microphone array, and the
// air (temperature, humidity). From these it derives, per octave band 63 Hz .. 16 kHz:
//   - reverberation time (Eyring, with the air term 4mV),
//   - the impulse response's early part: image sources (Allen & Berkley) with frequency-dependent
//     wall losses, scattering, air absorption, 1/r spreading and microphone directivity, plus the
//     scattered ("diffuse rain") energy growing in from the first reflection,
//   - the late-field energy envelope that the FDN tail is matched to,
//   - the room modes below the Schroeder frequency (Kuttruff's modal damping per wall pair),
//   - the direct paths (delay, 1/r, air, directivity) for the optional direct sound.
// Everything here runs when a room is built (worker thread), never per sample.
#include <array>
#include <cstdint>
#include <vector>

namespace cs {

constexpr int kSpaceBands = 9;                                   // octave bands
constexpr double kSpaceBandHz[kSpaceBands] = {63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};

struct Material {
    const char* name;
    double alpha[kSpaceBands];      // random-incidence energy absorption coefficient
    double scatter[kSpaceBands];    // scattering coefficient (share of reflected energy that is not specular)
};
int materialCount();
const Material& material(int index);
int materialIndex(const char* name);    // -1 if unknown
// Absorption and scattering of a surface with the room's furnishing applied.
double surfaceAlpha(const Material& m, int band, double furnishing);
double surfaceScatter(const Material& m, int band, double furnishing);

enum class MicArray { Ortf, Xy, Ab, WideAb, Blumlein, Mono };
constexpr int kMicArrayCount = 6;
const char* micArrayName(int i);

struct RoomSpec {
    double length = 24, width = 16, height = 10;   // metres (x: stage -> back, y: left -> right, z: up)
    int walls = 0, floor = 0, ceiling = 0;          // material indices (walls: all four)
    double distance = 8;        // source -> listener along the room, metres
    double offset = 0;          // listener across the room, -1 (left wall) .. 1 (right wall)
    double spread = 3;          // distance between the left and right sources, metres
    MicArray mic = MicArray::Ortf;
    double humidity = 50, temperature = 20;         // %, degrees C
    // Contents the shoebox does not model: furniture, people, doors, windows, irregular surfaces. 0 = an empty
    // box of the three materials; 1 = a fully furnished room. Adds absorption (fixtureAlpha) and scattering.
    double furnishing = 0.2;
    bool scattering = true;     // false: every reflection specular (no diffuse rain)
    int quality = 1;            // 0 Eco, 1 Normal, 2 High: early-part length, mode count
};

inline bool sameRoom(const RoomSpec& a, const RoomSpec& b) {
    return a.length == b.length && a.width == b.width && a.height == b.height && a.walls == b.walls && a.floor == b.floor &&
           a.ceiling == b.ceiling && a.distance == b.distance && a.offset == b.offset && a.spread == b.spread && a.mic == b.mic &&
           a.humidity == b.humidity && a.temperature == b.temperature && a.furnishing == b.furnishing && a.scattering == b.scattering && a.quality == b.quality;
}

// Derived acoustics.
struct RoomAcoustics {
    double c = 343;                       // speed of sound, m/s
    double volume = 0, surface = 0;       // m^3, m^2
    double meanFreePath = 0;              // 4V/S, metres
    double t60[kSpaceBands] = {};         // Eyring with air absorption, seconds
    double airDbPerM[kSpaceBands] = {};   // ISO 9613-1, dB/m
    double schroederHz = 0;               // 2000 sqrt(T/V)
    double mixingMs = 0;                  // perceptual mixing time (Lindau et al. 2012, tmp50 = 20 V/S + 12 ms)
    double criticalDistance = 0;          // r_c = 0.057 sqrt(V/T), Q = 1
    double scatterMean[kSpaceBands] = {}; // surface-weighted scattering
    double alphaMean[kSpaceBands] = {};   // surface-weighted absorption
};
RoomAcoustics analyseRoom(const RoomSpec& r);

// T60 at any frequency from the band values (log-frequency interpolation of the decay rate).
double spaceT60At(const RoomAcoustics& a, double f);

// FDN delay spread for the room's late field: around the mean free path (0.5x .. 1.6x, 3..150 ms).
void spaceFdnSpread(const RoomAcoustics& a, double& minMs, double& maxMs);

// ISO 9613-1 pure-tone air absorption, dB per metre.
double airAbsorptionDbPerM(double f, double tempC, double humidityPct, double pressureKpa = 101.325);

struct RoomMode {
    double freq = 0, decay = 0;     // Hz; amplitude decay rate delta (1/s): T60 = 6.91 / delta
    float in[2] = {};               // coupling from the left and right source
    float out[2] = {};              // coupling into the left and right microphone
};

struct DirectPath { double delaySec = 0; float gain = 0; float airAt8k = 1; };   // airAt8k: amplitude loss at 8 kHz

// Everything the engine needs, built once per room.
struct RoomRender {
    double fs = 48000;
    RoomAcoustics acoustics;
    // Early part, true stereo: [0] L->L, [1] L->R, [2] R->L, [3] R->R. Time 0 = first direct arrival.
    std::vector<std::vector<float>> early;
    double earlyMs = 0;                    // length of the early part (fades out over its last quarter)
    double tailStartMs = 0;                // when the matched FDN tail takes over (feed delay is set from this)
    // Late field: per-sample energy at time t (seconds after the direct sound) in band b for one source,
    // per microphone, in the same units as the early part: lateEnergy0[b] * exp(-13.8155 t / t60[b]).
    double lateEnergy0[kSpaceBands] = {};
    double fdnMinMs = 15, fdnMaxMs = 80;   // FDN delay spread from the room's mean free path
    double tailCoherence = 0;              // 0: independent L/R tail; up to ~0.6 for coincident pairs
    double modeCrossoverHz = 0;            // 0: no modal bank; else the tail is high-passed here and modes take over
    std::vector<RoomMode> modes;
    DirectPath direct[4];                  // same channel order as `early`
    double norm = 1;                       // output gain applied to everything so a room's reverb has unit energy
    double firstReflectionMs = 0;          // earliest image arrival after the first direct sound
    double source[2][3] = {}, mic[2][3] = {};   // placement (metres), for displays and tests
    double roomDims[3] = {};
};

RoomRender renderRoom(const RoomSpec& spec, double fs);

// For displays: the specular arrivals at the left microphone from the left source up to maxMs after the direct
// sound, as (milliseconds, dB relative to the direct sound at 1 kHz), strongest first, at most maxCount.
// Also the placement, as in RoomRender. Cheap (no band synthesis).
struct RoomSketch {
    std::vector<std::pair<float, float>> arrivals;
    double dims[3] = {}, source[2][3] = {}, mic[2][3] = {};
};
RoomSketch sketchRoom(const RoomSpec& spec, double maxMs, int maxCount);

}  // namespace cs
