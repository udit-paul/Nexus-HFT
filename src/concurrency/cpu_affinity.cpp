#include "nexus/concurrency/cpu_affinity.hpp"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#if defined(__linux__)
#include <sched.h>
#endif
#endif

namespace nexus {

bool pin_thread_to_core(int core_id) {
#if defined(_WIN32)
    DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << core_id);
    HANDLE thread = GetCurrentThread();
    return SetThreadAffinityMask(thread, mask) != 0;
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) == 0;
#else
    // macOS doesn't easily support strict thread pinning
    // Thread affinity APIs are private/undocumented on macOS
    return false;
#endif
}

bool pin_thread_to_core(std::thread& thread, int core_id) {
#if defined(_WIN32)
    DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << core_id);
    HANDLE handle = thread.native_handle();
    return SetThreadAffinityMask(handle, mask) != 0;
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t native_thread = thread.native_handle();
    return pthread_setaffinity_np(native_thread, sizeof(cpu_set_t), &cpuset) == 0;
#else
    return false;
#endif
}

} // namespace nexus
