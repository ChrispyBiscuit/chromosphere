#pragma once
// Flush denormals to zero on the calling thread for the lifetime of the object.
#if defined(__SSE__) || defined(_M_X64)
#include <xmmintrin.h>
#include <pmmintrin.h>
namespace fr {
class ScopedNoDenormals {
public:
    ScopedNoDenormals() : saved_(_mm_getcsr()) { _mm_setcsr(saved_ | 0x8040u); }  // FTZ | DAZ
    ~ScopedNoDenormals() { _mm_setcsr(saved_); }
private:
    unsigned saved_;
};
}
#elif defined(__aarch64__)
namespace fr {
class ScopedNoDenormals {
public:
    ScopedNoDenormals() {
        asm volatile("mrs %0, fpcr" : "=r"(saved_));
        unsigned long v = saved_ | (1ul << 24);  // FZ
        asm volatile("msr fpcr, %0" : : "r"(v));
    }
    ~ScopedNoDenormals() { asm volatile("msr fpcr, %0" : : "r"(saved_)); }
private:
    unsigned long saved_;
};
}
#else
namespace fr { class ScopedNoDenormals {}; }
#endif
