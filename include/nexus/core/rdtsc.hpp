#pragma once

#include <cstdint>
#include <cstddef>

// ── Platform includes (top-level, safe for all compilers) ─────────────────────
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <intrin.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <time.h>
#endif

namespace nexus {

/**
 * rdtsc.hpp — Low-overhead CPU cycle counter utilities
 *
 * RDTSC (Read Time-Stamp Counter) costs ~7–20 CPU cycles vs ~100–400 cycles
 * for std::chrono::high_resolution_clock (which bottoms out to a syscall).
 *
 * Use for hot-path latency measurement and benchmarking.
 *
 * Converting cycles → nanoseconds:
 *   ns = cycles / GHz_frequency
 *   Use RdtscCalibration::calibrate() once at startup.
 *
 * Note: Always pin threads (cpu_affinity.hpp) to ensure TSC consistency
 * across cores on the same socket.
 */

// ── Raw cycle counter ─────────────────────────────────────────────────────────

/**
 * Read the CPU cycle counter (RDTSC).
 * Lightweight — no serializing fence.
 */
[[nodiscard]] inline uint64_t rdtsc() noexcept {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    return static_cast<uint64_t>(__rdtsc());
#elif defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#else
    // Non-x86 fallback: nanosecond wall clock (less accurate)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
           + static_cast<uint64_t>(ts.tv_nsec);
#endif
}

/**
 * Read the CPU cycle counter with a serializing fence (RDTSCP).
 * Prevents CPU from reordering instructions across the measurement point.
 * Use for start/end boundaries of a precise timing region.
 */
[[nodiscard]] inline uint64_t rdtscp() noexcept {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    unsigned int ui = 0;
    return static_cast<uint64_t>(__rdtscp(&ui));
#elif defined(__x86_64__)
    uint32_t lo, hi, aux;
    __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#else
    return rdtsc();
#endif
}

// ── Calibration ───────────────────────────────────────────────────────────────

/**
 * RdtscCalibration — holds the pre-computed nanoseconds-per-cycle ratio.
 * Call calibrate() exactly once at program startup (before the hot path).
 */
struct RdtscCalibration {
    double ns_per_cycle{0.31};  ///< default: assume ~3.2 GHz until calibrated

    /**
     * Spin for ~10ms while sampling both RDTSC and a high-resolution
     * wall clock to compute the actual ns_per_cycle ratio.
     * Accuracy: ±0.1% on a quiet, affinity-pinned thread.
     */
    void calibrate() noexcept {
#if defined(_WIN32)
        LARGE_INTEGER freq, t1, t2;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&t1);
        uint64_t c1 = rdtsc();
        // Spin for ~10ms
        LARGE_INTEGER deadline;
        deadline.QuadPart = t1.QuadPart + freq.QuadPart / 100;
        do { QueryPerformanceCounter(&t2); } while (t2.QuadPart < deadline.QuadPart);
        uint64_t c2 = rdtsc();
        double elapsed_ns = static_cast<double>(t2.QuadPart - t1.QuadPart)
                            / static_cast<double>(freq.QuadPart) * 1e9;
        if (c2 > c1) {
            ns_per_cycle = elapsed_ns / static_cast<double>(c2 - c1);
        }
#else
        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        uint64_t c1 = rdtsc();
        struct timespec deadline = t1;
        deadline.tv_nsec += 10'000'000;
        if (deadline.tv_nsec >= 1'000'000'000) {
            deadline.tv_sec  += 1;
            deadline.tv_nsec -= 1'000'000'000;
        }
        do { clock_gettime(CLOCK_MONOTONIC, &t2); }
        while (t2.tv_sec < deadline.tv_sec ||
               (t2.tv_sec == deadline.tv_sec && t2.tv_nsec < deadline.tv_nsec));
        uint64_t c2 = rdtsc();
        double elapsed_ns = static_cast<double>(t2.tv_sec  - t1.tv_sec)  * 1e9
                          + static_cast<double>(t2.tv_nsec - t1.tv_nsec);
        if (c2 > c1) {
            ns_per_cycle = elapsed_ns / static_cast<double>(c2 - c1);
        }
#endif
    }

    /** Convert a raw cycle delta to nanoseconds (floating-point). */
    [[nodiscard]] double to_ns(uint64_t cycles) const noexcept {
        return static_cast<double>(cycles) * ns_per_cycle;
    }

    /** Convert a raw cycle delta to nanoseconds, rounded to nearest uint64. */
    [[nodiscard]] uint64_t to_ns_u64(uint64_t cycles) const noexcept {
        return static_cast<uint64_t>(to_ns(cycles) + 0.5);
    }
};

} // namespace nexus
