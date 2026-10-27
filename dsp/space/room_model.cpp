#include "space/room_model.h"
#include "common/rng.h"
#include "pffft.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cs {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDecay60 = 13.815510557964274;   // ln(10^6): energy decays by 60 dB over T60

// Octave-band data, 63 Hz .. 16 kHz. Absorption 125..4k from the classic compilation (Harris, Everest &
// Pohlmann, Egan); 63, 8k and 16k extrapolated. Scattering: Odeon's mid-frequency guidance (Zeng, Lynge,
// Rindel 2006) spread over the bands by surface group. Sources and confidence: docs/ACOUSTIC.md.
const Material kMaterials[] = {
    {"Painted concrete",  {.01, .01, .01, .02, .02, .02, .02, .03, .04}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Rough concrete",    {.02, .02, .03, .03, .03, .04, .07, .09, .10}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Concrete block",    {.12, .10, .05, .06, .07, .09, .08, .08, .09}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Cinder block",      {.30, .36, .44, .31, .29, .39, .25, .22, .20}, {.05, .05, .10, .15, .20, .30, .40, .45, .50}},
    {"Polished stone",    {.01, .01, .01, .01, .01, .02, .02, .03, .04}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Glazed tile",       {.01, .01, .01, .01, .01, .02, .02, .03, .04}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Brick",             {.03, .03, .03, .03, .04, .05, .07, .08, .10}, {.05, .05, .10, .15, .20, .30, .40, .45, .50}},
    {"Rough stone",       {.02, .02, .03, .03, .04, .05, .06, .07, .08}, {.10, .15, .25, .35, .45, .55, .60, .65, .70}},
    {"Plaster",           {.01, .013, .015, .02, .03, .04, .05, .06, .07}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Plaster on lath",   {.16, .14, .10, .06, .05, .04, .03, .03, .03}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Gypsum board",      {.30, .29, .10, .05, .04, .07, .09, .09, .10}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Wood panelling",    {.30, .28, .22, .17, .09, .10, .11, .11, .12}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Heavy wood",        {.15, .14, .10, .06, .08, .10, .10, .10, .11}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Wood floor",        {.16, .15, .11, .10, .07, .06, .07, .07, .08}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Parquet",           {.04, .04, .04, .07, .06, .06, .07, .07, .08}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Window glass",      {.40, .35, .25, .18, .12, .07, .04, .03, .03}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Plate glass",       {.20, .18, .06, .04, .03, .02, .02, .02, .03}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Sheet metal",       {.20, .15, .10, .08, .06, .05, .05, .05, .06}, {.05, .05, .10, .15, .20, .30, .40, .45, .50}},
    {"Carpet",            {.02, .02, .06, .14, .37, .60, .65, .65, .65}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Thick carpet",      {.05, .08, .24, .57, .69, .71, .73, .73, .73}, {.05, .05, .07, .10, .12, .15, .20, .25, .28}},
    {"Curtains",          {.08, .14, .35, .55, .72, .70, .65, .60, .58}, {.05, .10, .15, .20, .30, .40, .50, .50, .50}},
    {"Acoustic tile",     {.60, .76, .93, .83, .99, .99, .94, .90, .85}, {.05, .05, .10, .10, .15, .20, .25, .30, .30}},
    {"Mineral wool",      {.08, .17, .86, .99, .99, .99, .98, .95, .90}, {.05, .05, .10, .10, .15, .20, .25, .30, .30}},
    {"Audience",          {.45, .60, .74, .88, .96, .93, .85, .80, .75}, {.20, .30, .40, .50, .60, .70, .70, .70, .70}},
    {"Empty seats",       {.35, .49, .66, .80, .88, .82, .70, .65, .62}, {.20, .30, .40, .50, .60, .70, .70, .70, .70}},
    {"Bookshelves",       {.20, .25, .35, .40, .40, .40, .40, .40, .40}, {.10, .15, .20, .25, .30, .40, .50, .55, .60}},
    {"Water",             {.008, .008, .008, .013, .015, .020, .025, .03, .03}, {.01, .01, .02, .03, .05, .07, .10, .12, .14}},
    {"Open sky",          {.99, .99, .99, .99, .99, .99, .99, .99, .99}, {0, 0, 0, 0, 0, 0, 0, 0, 0}},
};
constexpr int kMaterialCount = (int)(sizeof(kMaterials) / sizeof(kMaterials[0]));

struct MicDef { const char* name; double spacing, angleDeg, pattern, coherence; };
// pattern: 0 omni, 0.5 cardioid, 1 figure-8 (first order: g = (1 - p) + p cos theta).
// coherence: frequency-averaged diffuse-field coherence of the pair, used for the late field.
const MicDef kMics[kMicArrayCount] = {
    {"ORTF", 0.17, 55.0, 0.5, 0.30},
    {"XY", 0.0, 45.0, 0.5, 0.75},
    {"AB", 0.6, 0.0, 0.0, 0.08},
    {"Wide AB", 2.0, 0.0, 0.0, 0.0},
    {"Blumlein", 0.0, 45.0, 1.0, 0.0},
    {"Mono", 0.0, 0.0, 0.0, 1.0},
};

struct Vec { double x, y, z; };
inline Vec sub(Vec a, Vec b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline double dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline double len(Vec a) { return std::sqrt(dot(a, a)); }

struct Geometry {
    double L, W, H;
    Vec src[2];                 // left, right source
    Vec mic[2];                 // left, right microphone
    Vec axis[2];                // unit look direction of each microphone
    double pattern = 0, coherence = 0;
};

Geometry place(const RoomSpec& r) {
    Geometry g;
    g.L = std::max(1.5, r.length); g.W = std::max(1.2, r.width); g.H = std::max(1.8, r.height);
    const double xs = std::clamp(0.18 * g.L, std::min(0.8, 0.3 * g.L), g.L - 0.5);
    const double zs = std::min(1.5, 0.6 * g.H), zl = std::min(1.6, 0.6 * g.H);
    const double halfSpread = std::min(0.5 * std::max(0.0, r.spread), 0.5 * g.W - 0.3);
    // The listener faces the stage (-x), so the left side is -y.
    g.src[0] = {xs, 0.5 * g.W - halfSpread, zs};
    g.src[1] = {xs, 0.5 * g.W + halfSpread, zs};
    const double xl = std::clamp(xs + std::max(0.3, r.distance), std::min(xs + 0.3, g.L - 0.3), g.L - 0.3);
    const double yl = 0.5 * g.W + std::clamp(r.offset, -1.0, 1.0) * std::max(0.0, 0.5 * g.W - 0.6);
    const MicDef& m = kMics[(int)r.mic];
    const double half = std::min(0.5 * m.spacing, 0.5 * g.W - 0.2), a = m.angleDeg * kPi / 180.0;
    g.mic[0] = {xl, std::max(0.2, yl - half), zl};
    g.mic[1] = {xl, std::min(g.W - 0.2, yl + half), zl};
    g.axis[0] = {-std::cos(a), -std::sin(a), 0};
    g.axis[1] = {-std::cos(a), std::sin(a), 0};
    g.pattern = m.pattern; g.coherence = m.coherence;
    return g;
}

inline double micGain(const Geometry& g, int m, Vec arrivalFrom) {   // arrivalFrom: from the microphone towards the (image) source
    const double d = len(arrivalFrom);
    const double c = d > 0 ? dot(g.axis[m], arrivalFrom) / d : 1.0;
    return (1.0 - g.pattern) + g.pattern * c;
}

// Image coordinate along one axis for image index i (i = 0: the source itself). Even i: translated
// copy; odd i: mirrored. Returns the number of hits on the low (0) and high (dim) wall.
inline double imageCoord(int i, double s, double dim, int& hitsLow, int& hitsHigh) {
    const int a = std::abs(i);
    if (i >= 0) { hitsHigh = (a + 1) / 2; hitsLow = a / 2; }
    else { hitsLow = (a + 1) / 2; hitsHigh = a / 2; }
    return i * dim + ((i & 1) ? dim - s : s);
}

// Smooth octave-band weights (partition of unity over log frequency) for the band-split synthesis.
double bandWeight(int b, double f) {
    if (f <= kSpaceBandHz[0]) return b == 0 ? 1.0 : 0.0;
    if (f >= kSpaceBandHz[kSpaceBands - 1]) return b == kSpaceBands - 1 ? 1.0 : 0.0;
    const double x = std::log2(f / kSpaceBandHz[0]);     // 0 .. kSpaceBands - 1
    const int k = std::min(kSpaceBands - 2, (int)x);
    const double t = x - k;
    if (b == k) return std::cos(0.5 * kPi * t) * std::cos(0.5 * kPi * t);
    if (b == k + 1) return std::sin(0.5 * kPi * t) * std::sin(0.5 * kPi * t);
    return 0.0;
}

double interpBand(const double* v, double f) {
    if (f <= kSpaceBandHz[0]) return v[0];
    if (f >= kSpaceBandHz[kSpaceBands - 1]) return v[kSpaceBands - 1];
    const double x = std::log2(f / kSpaceBandHz[0]);
    const int k = std::min(kSpaceBands - 2, (int)x);
    const double t = x - k;
    return v[k] + (v[k + 1] - v[k]) * t;
}
}  // namespace

// Absorption of the contents at full furnishing, per band: mixed furniture, a few people, a door and a window
// (their low-frequency share), spread over the surfaces as an equivalent coefficient.
constexpr double kFixtureAlpha[kSpaceBands] = {.20, .22, .25, .28, .30, .30, .30, .30, .30};

double surfaceAlpha(const Material& m, int b, double f) {
    const double k = std::clamp(f, 0.0, 1.0) * kFixtureAlpha[b];
    return 1.0 - (1.0 - m.alpha[b]) * (1.0 - k);
}
double surfaceScatter(const Material& m, int b, double f) {
    return 1.0 - (1.0 - m.scatter[b]) * (1.0 - 0.5 * std::clamp(f, 0.0, 1.0));
}

int materialCount() { return kMaterialCount; }
const Material& material(int i) { return kMaterials[std::clamp(i, 0, kMaterialCount - 1)]; }
int materialIndex(const char* name) {
    for (int i = 0; i < kMaterialCount; ++i) if (std::strcmp(kMaterials[i].name, name) == 0) return i;
    return -1;
}
const char* micArrayName(int i) { return kMics[std::clamp(i, 0, kMicArrayCount - 1)].name; }

double airAbsorptionDbPerM(double f, double tempC, double hr, double paKpa) {
    // ISO 9613-1:1993, equations (3) to (5) and Annex B for the molar concentration of water vapour.
    const double T = tempC + 273.15, T0 = 293.15, T01 = 273.16, pr = 101.325, pa = paKpa;
    const double C = -6.8346 * std::pow(T01 / T, 1.261) + 4.6151;
    const double h = std::clamp(hr, 1.0, 100.0) * std::pow(10.0, C) * (pr / pa);
    const double frO = (pa / pr) * (24.0 + 4.04e4 * h * (0.02 + h) / (0.391 + h));
    const double frN = (pa / pr) * std::pow(T / T0, -0.5) * (9.0 + 280.0 * h * std::exp(-4.170 * (std::pow(T / T0, -1.0 / 3.0) - 1.0)));
    const double f2 = f * f;
    return 8.686 * f2 * (1.84e-11 * (pr / pa) * std::sqrt(T / T0) +
                         std::pow(T / T0, -2.5) * (0.01275 * std::exp(-2239.1 / T) / (frO + f2 / frO) +
                                                   0.1068 * std::exp(-3352.0 / T) / (frN + f2 / frN)));
}

RoomAcoustics analyseRoom(const RoomSpec& r) {
    RoomAcoustics a;
    const Geometry g = place(r);
    a.c = 331.3 * std::sqrt(1.0 + r.temperature / 273.15);
    a.volume = g.L * g.W * g.H;
    const double sFloor = g.L * g.W, sCeil = g.L * g.W, sWalls = 2.0 * (g.L * g.H + g.W * g.H);
    a.surface = sFloor + sCeil + sWalls;
    a.meanFreePath = 4.0 * a.volume / a.surface;
    const Material& mw = material(r.walls), &mf = material(r.floor), &mc = material(r.ceiling);
    for (int b = 0; b < kSpaceBands; ++b) {
        a.airDbPerM[b] = airAbsorptionDbPerM(kSpaceBandHz[b], r.temperature, r.humidity);
        const double m = a.airDbPerM[b] / (10.0 * std::log10(std::exp(1.0)));   // energy attenuation, 1/m
        const double F = r.furnishing;
        const double A = sFloor * surfaceAlpha(mf, b, F) + sCeil * surfaceAlpha(mc, b, F) + sWalls * surfaceAlpha(mw, b, F);
        a.alphaMean[b] = std::clamp(A / a.surface, 1e-4, 0.995);
        a.scatterMean[b] = r.scattering ? (sFloor * surfaceScatter(mf, b, F) + sCeil * surfaceScatter(mc, b, F) + sWalls * surfaceScatter(mw, b, F)) / a.surface : 0.0;
        // Eyring with the air term: T = 24 ln10 V / (c (-S ln(1 - a) + 4 m V)).
        a.t60[b] = 24.0 * std::log(10.0) * a.volume / (a.c * (-a.surface * std::log(1.0 - a.alphaMean[b]) + 4.0 * m * a.volume));
        a.t60[b] = std::clamp(a.t60[b], 0.03, 60.0);
    }
    const double tMid = 0.5 * (a.t60[3] + a.t60[4]);
    a.schroederHz = 2000.0 * std::sqrt(tMid / a.volume);
    a.mixingMs = 20.0 * a.volume / a.surface + 12.0;
    a.criticalDistance = 0.057 * std::sqrt(a.volume / tMid);
    return a;
}

double spaceT60At(const RoomAcoustics& a, double f) {
    double rate[kSpaceBands];
    for (int b = 0; b < kSpaceBands; ++b) rate[b] = 1.0 / a.t60[b];
    return 1.0 / interpBand(rate, f);
}

RoomSketch sketchRoom(const RoomSpec& spec, double maxMs, int maxCount) {
    RoomSketch out;
    const Geometry g = place(spec);
    const RoomAcoustics A = analyseRoom(spec);
    const double c = A.c;
    const Material* surf[6] = {&material(spec.walls), &material(spec.walls), &material(spec.walls), &material(spec.walls),
                               &material(spec.floor), &material(spec.ceiling)};
    double lnR[6];
    for (int k = 0; k < 6; ++k) lnR[k] = 0.5 * std::log(std::max(1e-6, (1.0 - surfaceAlpha(*surf[k], 4, spec.furnishing)) *
                                                                       (1.0 - (spec.scattering ? surfaceScatter(*surf[k], 4, spec.furnishing) : 0.0))));
    const Vec S = g.src[0], M = g.mic[0];
    const double dDirect = len(sub(S, M)), maxPath = dDirect + c * maxMs / 1000.0;
    const double direct = std::fabs(micGain(g, 0, sub(S, M))) / std::max(dDirect, 0.1);
    const int Ix = (int)std::ceil(maxPath / g.L) + 1, Iy = (int)std::ceil(maxPath / g.W) + 1, Iz = (int)std::ceil(maxPath / g.H) + 1;
    for (int ix = -Ix; ix <= Ix; ++ix) {
        int hx0, hx1; const double x = imageCoord(ix, S.x, g.L, hx0, hx1);
        for (int iy = -Iy; iy <= Iy; ++iy) {
            int hy0, hy1; const double y = imageCoord(iy, S.y, g.W, hy0, hy1);
            for (int iz = -Iz; iz <= Iz; ++iz) {
                if (ix == 0 && iy == 0 && iz == 0) continue;
                int hz0, hz1; const double z = imageCoord(iz, S.z, g.H, hz0, hz1);
                const Vec dv = sub({x, y, z}, M);
                const double d = len(dv);
                if (d > maxPath) continue;
                const double lnWall = hx0 * lnR[0] + hx1 * lnR[1] + hy0 * lnR[2] + hy1 * lnR[3] + hz0 * lnR[4] + hz1 * lnR[5];
                const double amp = std::fabs(micGain(g, 0, dv)) / d * std::exp(lnWall - A.airDbPerM[4] / 8.686 * d);
                out.arrivals.push_back({(float)(1000.0 * (d - dDirect) / c), (float)(20.0 * std::log10(std::max(amp / direct, 1e-6)))});
            }
        }
    }
    std::sort(out.arrivals.begin(), out.arrivals.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    if ((int)out.arrivals.size() > maxCount) out.arrivals.resize((size_t)maxCount);
    out.dims[0] = g.L; out.dims[1] = g.W; out.dims[2] = g.H;
    for (int k = 0; k < 2; ++k) {
        out.source[k][0] = g.src[k].x; out.source[k][1] = g.src[k].y; out.source[k][2] = g.src[k].z;
        out.mic[k][0] = g.mic[k].x; out.mic[k][1] = g.mic[k].y; out.mic[k][2] = g.mic[k].z;
    }
    return out;
}

void spaceFdnSpread(const RoomAcoustics& a, double& minMs, double& maxMs) {
    const double mfpMs = 1000.0 * a.meanFreePath / a.c;
    minMs = std::clamp(0.5 * mfpMs, 3.0, 60.0);
    maxMs = std::clamp(1.6 * mfpMs, minMs + 4.0, 150.0);
}

RoomRender renderRoom(const RoomSpec& spec, double fs) {
    RoomRender out;
    out.fs = fs;
    const Geometry g = place(spec);
    const RoomAcoustics& A = out.acoustics = analyseRoom(spec);
    const double c = A.c, V = A.volume;
    const Material* surf[6] = {&material(spec.walls), &material(spec.walls), &material(spec.walls), &material(spec.walls),
                               &material(spec.floor), &material(spec.ceiling)};   // x0, xL, y0, yW, z0 (floor), zH (ceiling)
    const double dfEff = (1.0 - g.pattern) * (1.0 - g.pattern) + g.pattern * g.pattern / 3.0;   // diffuse-field energy response of the pattern
    out.tailCoherence = g.coherence;

    // Time origin: the first direct arrival over the four source/microphone pairs.
    double dMin = 1e9;
    for (int s = 0; s < 2; ++s) for (int m = 0; m < 2; ++m) dMin = std::min(dMin, len(sub(g.src[s], g.mic[m])));
    const double t0 = dMin / c;

    // Early-part length: at least the conservative perceptual mixing time (tmp95 = 0.0117 V + 50.1 ms,
    // Lindau et al.) and two mean ones, 50..220 ms at Normal quality, never much longer than the decay itself.
    const double qScale = spec.quality == 0 ? 0.6 : spec.quality == 2 ? 1.4 : 1.0;
    double earlyMs = std::clamp(std::max(0.0117 * V + 50.1, 2.0 * A.mixingMs), 50.0, 220.0) * qScale;
    earlyMs = std::clamp(earlyMs, 30.0, std::max(40.0, 0.6 * 1000.0 * A.t60[4]));
    out.earlyMs = earlyMs;
    const int N = (int)std::ceil(earlyMs * fs / 1000.0);
    constexpr int kPad = 512;                        // room for the zero-phase band smoothing before t = 0
    int nfft = 64; while (nfft < N + 2 * kPad) nfft *= 2;

    // Per-surface, per-band log reflection factor of the specular part: sqrt((1 - alpha)(1 - s)).
    double lnR[6][kSpaceBands];
    for (int k = 0; k < 6; ++k)
        for (int b = 0; b < kSpaceBands; ++b) {
            const double s = spec.scattering ? surfaceScatter(*surf[k], b, spec.furnishing) : 0.0;
            lnR[k][b] = 0.5 * std::log(std::max(1e-6, (1.0 - surfaceAlpha(*surf[k], b, spec.furnishing)) * (1.0 - s)));
        }
    double airNp[kSpaceBands];                       // amplitude attenuation, nepers per metre
    for (int b = 0; b < kSpaceBands; ++b) airNp[b] = A.airDbPerM[b] / 8.686;

    std::vector<std::vector<float>> band((size_t)2 * kSpaceBands, std::vector<float>((size_t)nfft, 0.f));   // [mic * bands + b]
    std::vector<float> spec2((size_t)nfft), work((size_t)nfft), acc((size_t)nfft), tmp((size_t)nfft);
    PFFFT_Setup* fft = pffft_new_setup(nfft, PFFFT_REAL);
    out.early.assign(4, std::vector<float>((size_t)N, 0.f));
    const double maxPath = c * (t0 + earlyMs / 1000.0);
    Rng rng(0x5eed + (uint64_t)(V * 10.0));
    double firstRefl = 1e9;

    for (int s = 0; s < 2; ++s) {
        for (auto& v : band) std::fill(v.begin(), v.end(), 0.f);
        const Vec S = g.src[s];
        // --- Specular image sources. ---
        const int Ix = (int)std::ceil(maxPath / g.L) + 1, Iy = (int)std::ceil(maxPath / g.W) + 1, Iz = (int)std::ceil(maxPath / g.H) + 1;
        for (int ix = -Ix; ix <= Ix; ++ix) {
            int hx0, hx1; const double x = imageCoord(ix, S.x, g.L, hx0, hx1);
            for (int iy = -Iy; iy <= Iy; ++iy) {
                int hy0, hy1; const double y = imageCoord(iy, S.y, g.W, hy0, hy1);
                for (int iz = -Iz; iz <= Iz; ++iz) {
                    if (ix == 0 && iy == 0 && iz == 0) continue;          // the direct sound is separate
                    int hz0, hz1; const double z = imageCoord(iz, S.z, g.H, hz0, hz1);
                    const int hits[6] = {hx0, hx1, hy0, hy1, hz0, hz1};
                    double lnWall[kSpaceBands];
                    double best = -1e9;
                    for (int b = 0; b < kSpaceBands; ++b) {
                        double v = 0; for (int k = 0; k < 6; ++k) v += hits[k] * lnR[k][b];
                        lnWall[b] = v; best = std::max(best, v);
                    }
                    if (best < std::log(1e-5)) continue;                   // too weak to matter
                    for (int m = 0; m < 2; ++m) {
                        const Vec dv = sub({x, y, z}, g.mic[m]);
                        const double d = len(dv);
                        if (d > maxPath) continue;
                        const double t = d / c - t0;
                        if (t >= earlyMs / 1000.0) continue;
                        firstRefl = std::min(firstRefl, t);
                        const double gm = micGain(g, m, dv) / std::max(d, 0.1);
                        // 8-tap Hann-windowed sinc fractional delay.
                        const double pos = kPad + t * fs;
                        const int i0 = (int)std::floor(pos) - 3;
                        const double frac = pos - std::floor(pos);
                        float ker[8];
                        for (int k = 0; k < 8; ++k) {
                            const double u = (k - 3) - frac;                  // tap offset from the exact position
                            const double sinc = std::fabs(u) < 1e-9 ? 1.0 : std::sin(kPi * u) / (kPi * u);
                            const double w = 0.5 + 0.5 * std::cos(kPi * u / 4.0);
                            ker[k] = (float)(sinc * w);
                        }
                        if (i0 < 0 || i0 + 8 > nfft) continue;
                        for (int b = 0; b < kSpaceBands; ++b) {
                            const float amp = (float)(gm * std::exp(lnWall[b] - airNp[b] * d));
                            float* dst = band[(size_t)(m * kSpaceBands + b)].data() + i0;
                            for (int k = 0; k < 8; ++k) dst[k] += amp * ker[k];
                        }
                    }
                }
            }
        }
        // --- Scattered energy ("diffuse rain"): impulses of growing density from the first reflection on,
        // carrying the share of the statistical energy that has been scattered at least once. ---
        if (spec.scattering) {
            const double tFirst = std::max(0.0, (2.0 * std::min({S.z, g.mic[0].z, g.H - S.z})) / c - 0.0005);   // about the floor reflection
            const double a = std::cos(0.5 * std::asin(std::clamp(g.coherence, 0.0, 1.0))), bmix = std::sin(0.5 * std::asin(std::clamp(g.coherence, 0.0, 1.0)));
            for (int seq = 0; seq < 2; ++seq) {
                const float wL = (float)(seq == 0 ? a : bmix), wR = (float)(seq == 0 ? bmix : a);
                for (int i = (int)(tFirst * fs); i < N; ++i) {
                    const double tau = t0 + (double)i / fs;                  // time since emission
                    const double density = 4.0 * kPi * c * c * c * tau * tau / V;   // reflections per second
                    const double p = std::min(1.0, density / fs);
                    if (rng.uniform() >= p) continue;
                    const double nRefl = c * tau / A.meanFreePath;
                    const float sign = (rng.next() & 1) ? 1.f : -1.f;
                    for (int b = 0; b < kSpaceBands; ++b) {
                        const double scattered = 1.0 - std::pow(1.0 - A.scatterMean[b], nRefl);
                        const double e = (4.0 * kPi * c / V) * std::exp(-kDecay60 * tau / A.t60[b]) * scattered * dfEff / fs;   // energy per sample
                        const float amp = sign * (float)std::sqrt(e / p);
                        band[(size_t)b][(size_t)(kPad + i)] += wL * amp;
                        band[(size_t)(kSpaceBands + b)][(size_t)(kPad + i)] += wR * amp;
                    }
                }
            }
        }
        // --- Band filtering: each band's impulses weighted by its smooth octave window, summed. ---
        for (int m = 0; m < 2; ++m) {
            std::fill(acc.begin(), acc.end(), 0.f);
            for (int b = 0; b < kSpaceBands; ++b) {
                pffft_transform_ordered(fft, band[(size_t)(m * kSpaceBands + b)].data(), spec2.data(), work.data(), PFFFT_FORWARD);
                // Ordered real layout: [DC, Nyquist, re1, im1, re2, im2, ...].
                acc[0] += spec2[0] * (float)bandWeight(b, 0.0);
                acc[1] += spec2[1] * (float)bandWeight(b, 0.5 * fs);
                for (int k = 1; k < nfft / 2; ++k) {
                    const float w = (float)bandWeight(b, k * fs / nfft);
                    if (w == 0.f) continue;
                    acc[(size_t)(2 * k)] += w * spec2[(size_t)(2 * k)];
                    acc[(size_t)(2 * k + 1)] += w * spec2[(size_t)(2 * k + 1)];
                }
            }
            pffft_transform_ordered(fft, acc.data(), tmp.data(), work.data(), PFFFT_BACKWARD);
            auto& dst = out.early[(size_t)(s * 2 + m)];
            for (int i = 0; i < N; ++i) dst[(size_t)i] = tmp[(size_t)(kPad + i)] / (float)nfft;
        }
    }
    pffft_destroy_setup(fft);

    out.firstReflectionMs = firstRefl < 1e8 ? 1000.0 * firstRefl : 0.0;
    for (int k = 0; k < 2; ++k) {
        out.source[k][0] = g.src[k].x; out.source[k][1] = g.src[k].y; out.source[k][2] = g.src[k].z;
        out.mic[k][0] = g.mic[k].x; out.mic[k][1] = g.mic[k].y; out.mic[k][2] = g.mic[k].z;
    }
    out.roomDims[0] = g.L; out.roomDims[1] = g.W; out.roomDims[2] = g.H;

    // Fade the early part out over its last quarter (the tail takes over there).
    const int fadeN = std::max(1, N / 4);
    for (auto& ch : out.early)
        for (int i = 0; i < fadeN; ++i) ch[(size_t)(N - fadeN + i)] *= (float)(0.5 + 0.5 * std::cos(kPi * (i + 1.0) / fadeN));

    // --- Late field (matched by the engine's FDN). ---
    for (int b = 0; b < kSpaceBands; ++b) out.lateEnergy0[b] = (4.0 * kPi * c / V) * dfEff / fs * std::exp(-kDecay60 * t0 / A.t60[b]);
    spaceFdnSpread(A, out.fdnMinMs, out.fdnMaxMs);
    out.tailStartMs = 0.75 * earlyMs;

    // --- Room modes below the Schroeder frequency (small rooms only). ---
    out.modeCrossoverHz = 0;
    if (A.schroederHz > 70.0) {
        const double fc = std::min(A.schroederHz, 250.0);
        const int maxModes = spec.quality == 0 ? 24 : spec.quality == 2 ? 64 : 48;
        const Material& mw = material(spec.walls), &mf = material(spec.floor), &mcl = material(spec.ceiling);
        std::vector<RoomMode> modes;
        const int nxMax = (int)(2.0 * g.L * fc * 1.15 / c) + 1, nyMax = (int)(2.0 * g.W * fc * 1.15 / c) + 1, nzMax = (int)(2.0 * g.H * fc * 1.15 / c) + 1;
        for (int nx = 0; nx <= nxMax; ++nx)
            for (int ny = 0; ny <= nyMax; ++ny)
                for (int nz = 0; nz <= nzMax; ++nz) {
                    if (nx + ny + nz == 0) continue;
                    const double f = 0.5 * c * std::sqrt((nx / g.L) * (nx / g.L) + (ny / g.W) * (ny / g.W) + (nz / g.H) * (nz / g.H));
                    if (f > fc * 1.15 || f < 20.0) continue;
                    // Modal damping (Kuttruff): delta = c / 8V * sum over wall pairs of eps * alpha * area, eps = 1 if
                    // the mode has a component normal to the pair, 1/2 if it runs parallel to it; plus air.
                    auto al = [&](const Material& m) {
                        double v[kSpaceBands]; for (int b = 0; b < kSpaceBands; ++b) v[b] = surfaceAlpha(m, b, spec.furnishing);
                        return interpBand(v, f);
                    };
                    const double Ax = 2.0 * g.W * g.H * al(mw), Ay = 2.0 * g.L * g.H * al(mw), Az = g.L * g.W * (al(mf) + al(mcl));
                    const double delta = c / (8.0 * V) * ((nx ? 1.0 : 0.5) * Ax + (ny ? 1.0 : 0.5) * Ay + (nz ? 1.0 : 0.5) * Az) +
                                         0.5 * c * airAbsorptionDbPerM(f, spec.temperature, spec.humidity) / 4.343;
                    RoomMode md; md.freq = f; md.decay = delta;
                    const double eps = (nx ? 2.0 : 1.0) * (ny ? 2.0 : 1.0) * (nz ? 2.0 : 1.0);
                    auto psi = [&](Vec p) { return std::cos(nx * kPi * p.x / g.L) * std::cos(ny * kPi * p.y / g.W) * std::cos(nz * kPi * p.z / g.H); };
                    for (int k = 0; k < 2; ++k) { md.in[k] = (float)(std::sqrt(eps) * psi(g.src[k])); md.out[k] = (float)(std::sqrt(eps) * psi(g.mic[k])); }
                    modes.push_back(md);
                }
        std::sort(modes.begin(), modes.end(), [](const RoomMode& x, const RoomMode& y) { return x.freq < y.freq; });
        if ((int)modes.size() > maxModes) modes.resize((size_t)maxModes);
        if (!modes.empty()) {
            const double fTop = modes.back().freq;
            out.modeCrossoverHz = std::min(fc, fTop);
            // Scale: total modal energy (both sources, both mics, averaged) equals the statistical late energy
            // below the crossover, i.e. that share of a white source's spectrum.
            double eModes = 0;
            for (const RoomMode& md : modes) {
                const double r = std::exp(-md.decay / fs);
                const double e1 = 1.0 / (2.0 * (1.0 - r * r));
                for (int s = 0; s < 2; ++s) for (int m = 0; m < 2; ++m) eModes += (double)md.in[s] * md.in[s] * md.out[m] * md.out[m] * e1;
            }
            eModes /= 4.0;
            const double tLow = interpBand(A.t60, 0.5 * out.modeCrossoverHz);
            const double share = out.modeCrossoverHz / (0.5 * fs);
            const double target = out.lateEnergy0[1] * fs * tLow / kDecay60 * share;
            const double k = eModes > 0 ? std::sqrt(target / eModes) : 0.0;
            for (RoomMode& md : modes) for (int s = 0; s < 2; ++s) md.in[s] = (float)(md.in[s] * std::sqrt(k)), md.out[s] = (float)(md.out[s] * std::sqrt(k));
            out.modes = std::move(modes);
        }
    }

    // --- Direct paths. ---
    for (int s = 0; s < 2; ++s)
        for (int m = 0; m < 2; ++m) {
            const Vec dv = sub(g.src[s], g.mic[m]);
            const double d = len(dv);
            DirectPath& p = out.direct[s * 2 + m];
            p.delaySec = d / c - t0;
            p.gain = (float)(micGain(g, m, dv) / std::max(d, 0.1));
            p.airAt8k = (float)std::pow(10.0, -airAbsorptionDbPerM(8000.0, spec.temperature, spec.humidity) * d / 20.0);
        }

    // --- Normalisation: the reverb (early part + late field) of one source carries unit energy per mic. ---
    double eEarly = 0;
    for (const auto& ch : out.early) for (float v : ch) eEarly += (double)v * v;
    eEarly /= 4.0;
    const double tMid = A.t60[4];
    const double eLate = out.lateEnergy0[4] * fs * tMid / kDecay60 * std::exp(-kDecay60 * (earlyMs / 1000.0) / tMid);
    out.norm = 1.0 / std::sqrt(std::max(1e-12, eEarly + eLate));
    for (auto& ch : out.early) for (float& v : ch) v *= (float)out.norm;
    for (RoomMode& md : out.modes) for (int s = 0; s < 2; ++s) md.in[s] = (float)(md.in[s] * out.norm);
    for (DirectPath& p : out.direct) p.gain = (float)(p.gain * out.norm);
    return out;
}

}  // namespace cs
