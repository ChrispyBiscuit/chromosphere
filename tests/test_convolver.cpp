#include "conv/partitioned_convolver.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static std::vector<float> noise(size_t n, unsigned seed, float amp) {
    std::mt19937 r(seed); std::vector<float> v(n);
    for (float& x : v) x = ((r() >> 8) * (1.0f / 8388608.f) - 1.f) * amp;
    return v;
}
// Reference: direct convolution in double.
static std::vector<double> direct(const std::vector<float>& x, const std::vector<float>& h, size_t outLen) {
    std::vector<double> y(outLen, 0.0);
    for (size_t n = 0; n < outLen; ++n) {
        double s = 0; size_t kmax = std::min(n + 1, h.size());
        for (size_t k = 0; k < kmax; ++k) s += (double)h[k] * x[n - k];
        y[n] = s;
    }
    return y;
}

// Run through the convolver with the given host block sizes (cycled), return L and R outputs.
static void run(fr::PartitionedConvolver& cv, const std::vector<float>& xl, const std::vector<float>& xr,
                std::vector<float>& yl, std::vector<float>& yr, const std::vector<int>& blocks) {
    const size_t n = xl.size();
    yl.assign(n, 0.f); yr.assign(n, 0.f);
    size_t pos = 0, bi = 0;
    while (pos < n) {
        size_t b = std::min<size_t>(blocks[bi++ % blocks.size()], n - pos);
        cv.process(&xl[pos], &xr[pos], &yl[pos], &yr[pos], (int)b);
        pos += b;
    }
}

int main() {
    // 1. Matches direct convolution for several IR lengths (covers every level boundary) and block sizes.
    for (int irLen : {1, 40, 64, 65, 255, 256, 257, 1000, 1024, 4095, 4096, 4097, 9000, 30000, 70000}) {
        auto h = noise(irLen, 7 + irLen, 0.5f);
        const size_t n = std::max<size_t>(irLen * 2 + 5000, 20000);
        auto xl = noise(n, 1, 0.5f), xr = noise(n, 2, 0.5f);
        fr::PartitionedConvolver cv; cv.configure({h});            // mono IR: same filter on both channels
        for (const std::vector<int>& blocks : {std::vector<int>{128}, std::vector<int>{1, 7, 513, 64, 3000}}) {
            std::vector<float> yl, yr; cv.reset(); run(cv, xl, xr, yl, yr, blocks);
            auto rl = direct(xl, h, n), rr = direct(xr, h, n);
            double e = 0, ref = 0;
            for (size_t i = 0; i < n; ++i) { e = std::max({e, std::fabs(yl[i] - rl[i]), std::fabs(yr[i] - rr[i])}); ref = std::max(ref, std::fabs(rl[i])); }
            CHECK(e < 2e-5 * std::max(1.0, ref), "irLen %d blocks[0]=%d: max error %g (ref peak %g)", irLen, blocks[0], e, ref);
        }
    }
    // 2. Zero latency: an impulse in gives h[0] at the same sample.
    {
        std::vector<float> h = {0.75f, 0.1f, -0.2f}; h.resize(6000, 0.01f);
        fr::PartitionedConvolver cv; cv.configure({h});
        std::vector<float> x(300, 0.f), y(300), y2(300); x[0] = 1.f;
        cv.process(x.data(), x.data(), y.data(), y2.data(), 300);
        CHECK(std::fabs(y[0] - 0.75f) < 1e-6f && std::fabs(y[1] - 0.1f) < 1e-6f, "latency: y[0]=%g y[1]=%g", y[0], y[1]);
    }
    // 3. Stereo and true-stereo routing.
    {
        auto a = noise(5000, 11, 0.3f), b = noise(5000, 12, 0.3f), c = noise(5000, 13, 0.3f), d = noise(5000, 14, 0.3f);
        const size_t n = 12000;
        auto xl = noise(n, 3, 0.5f), xr = noise(n, 4, 0.5f);
        std::vector<float> yl, yr;
        fr::PartitionedConvolver st; st.configure({a, b}); run(st, xl, xr, yl, yr, {200});
        auto rl = direct(xl, a, n), rr = direct(xr, b, n);
        double e = 0; for (size_t i = 0; i < n; ++i) e = std::max({e, std::fabs(yl[i] - rl[i]), std::fabs(yr[i] - rr[i])});
        CHECK(e < 1e-4, "stereo routing error %g", e);
        fr::PartitionedConvolver ts; ts.configure({a, b, c, d}); run(ts, xl, xr, yl, yr, {200});   // LL, LR, RL, RR
        auto ll = direct(xl, a, n), lr = direct(xl, b, n), rl2 = direct(xr, c, n), rr2 = direct(xr, d, n);
        e = 0; for (size_t i = 0; i < n; ++i) e = std::max({e, std::fabs(yl[i] - (ll[i] + rl2[i])), std::fabs(yr[i] - (lr[i] + rr2[i]))});
        CHECK(e < 1e-4, "true-stereo routing error %g", e);
    }
    // 4. In-place processing and silence.
    {
        auto h = noise(9000, 21, 0.4f);
        fr::PartitionedConvolver cv; cv.configure({h});
        auto x = noise(15000, 5, 0.5f), z = x; std::vector<float> yl, yr;
        run(cv, x, z, yl, yr, {256});
        cv.reset();
        std::vector<float> a = x, b = x;
        for (size_t p = 0; p < a.size(); p += 100) { int c = (int)std::min<size_t>(100, a.size() - p); cv.process(&a[p], &b[p], &a[p], &b[p], c); }
        double e = 0; for (size_t i = 0; i < a.size(); ++i) e = std::max(e, (double)std::fabs(a[i] - yl[i]));
        CHECK(e < 1e-6, "in-place differs by %g", e);
        std::vector<float> zero(5000, 0.f), o1(5000), o2(5000); cv.reset();
        cv.process(zero.data(), zero.data(), o1.data(), o2.data(), 5000);
        float m = 0; for (float v : o1) m = std::max(m, std::fabs(v));
        CHECK(m == 0.f, "silence in gave %g", m);
    }
    // 5. Worst-block time for a 10 s stereo IR at 48 kHz, 128-sample blocks.
    {
        auto h1 = noise(480000, 31, 0.1f), h2 = noise(480000, 32, 0.1f);
        for (size_t i = 0; i < h1.size(); ++i) { float e = std::exp(-3.0f * i / 48000.f); h1[i] *= e; h2[i] *= e; }
        fr::PartitionedConvolver cv; cv.configure({h1, h2});
        auto x = noise(48000 * 6, 9, 0.3f); std::vector<float> y(x.size()), y2(x.size());
        std::vector<double> t; t.reserve(x.size() / 128);
        for (size_t p = 0; p + 128 <= x.size(); p += 128) {
            auto t0 = std::chrono::steady_clock::now();
            cv.process(&x[p], &x[p], &y[p], &y2[p], 128);
            t.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
        }
        double total = 0, worst = 0; for (double v : t) { total += v; worst = std::max(worst, v); }
        std::sort(t.begin(), t.end());
        std::printf("10 s stereo IR: avg %.2f%% core, p99.9 block %.1f%%, worst block %.1f%% of 2.67 ms, memory %.1f MB\n",
                    100 * total / (x.size() / 48000.0), 100 * t[t.size() * 999 / 1000] / (128 / 48000.0), 100 * worst / (128 / 48000.0), cv.memoryBytes() / 1e6);
    }
    std::printf(failures ? "%d failure(s)\n" : "convolver tests passed\n", failures);
    return failures ? 1 : 0;
}
