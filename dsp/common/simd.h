#pragma once
// Runtime CPU dispatch for hot loops.
//
// CS_CLONES marks a function for compilation in several instruction-set variants; the
// loader picks the best one for the running CPU the first time it is called.
//  - x86-64 GCC/Clang on Linux: AVX-512, AVX2+FMA, and a baseline (SSE2) version.
//  - Other targets: one version. Apple Silicon and AArch64 get NEON from the baseline.
//    Windows/MSVC dispatch is not done yet (needs per-file /arch flags; see PRD M6).
// Results can differ in the last bits between variants (FMA, reduction order), so the
// bit-exact determinism gate (Q11) applies per CPU. Define CS_NO_CLONES to force one variant.
#if defined(__x86_64__) && defined(__linux__) && defined(__GNUC__) && !defined(CS_NO_CLONES)
#define CS_CLONES __attribute__((target_clones("arch=skylake-avx512", "arch=haswell", "default")))
#else
#define CS_CLONES
#endif
