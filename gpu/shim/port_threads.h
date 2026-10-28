// SPDX-License-Identifier: GPL-2.0-or-later
// port: thread topology and scheduling optimizations for Intel Xeon E5-2697 v3
// (Haswell-EP, 14 Cores / 28 Threads) and multi-core hosts:
// - 2 to 4 dedicated threads: Main Loop, Render Submission, Audio, and I/O.
// - 24 to 26 worker threads: Background async shader compilation and asset decompression.
// - Dedicated affinity masks (SetThreadAffinityMask) and priority tiers.

#pragma once

#include <algorithm>
#include <cstdlib>
#include <thread>
#ifdef _WIN32
#include <bit>
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace PortThreads {

#ifdef _WIN32
/// Hardware threads available to the process (its affinity mask).
inline unsigned Available() {
    DWORD_PTR process = 0, system = 0;
    if (GetProcessAffinityMask(GetCurrentProcess(), &process, &system) && process) {
        return std::max(1, std::popcount(static_cast<unsigned long long>(process)));
    }
    return std::max(1u, std::thread::hardware_concurrency());
}

/// Number of dedicated system threads (Main Loop, Render Submission, Audio, I/O).
/// Scales dynamically: 4 on >= 16 threads (e.g. 28t Xeon E5-2697 v3), 2 on >= 8 threads, 1 otherwise.
inline unsigned DedicatedThreads() {
    const unsigned total = Available();
    if (total >= 16) return 4u;
    if (total >= 8)  return 2u;
    return 1u;
}

/// Number of dedicated worker threads for async shader compilation and asset streaming.
/// On Intel Xeon E5-2697 v3 (28 threads): 28 - 4 = 24 worker threads.
inline unsigned WorkerThreads() {
    if (const char* env = std::getenv("THEORDER_WORKER_THREADS")) {
        int custom = std::atoi(env);
        if (custom > 0) return static_cast<unsigned>(custom);
    }
    if (const char* xeon = std::getenv("THEORDER_XEON_MODE")) {
        if (std::strcmp(xeon, "0") == 0 || std::strcmp(xeon, "off") == 0) {
            return 4u;
        }
    }
    const unsigned total = Available();
    const unsigned dedicated = DedicatedThreads();
    return total > dedicated ? (total - dedicated) : 1u;
}

/// Worker threads for background async shader compilation.
inline unsigned AsyncShaderWorkers() {
    return WorkerThreads();
}

/// Combined affinity mask for dedicated threads (threads 0..3 on >= 16t systems)
inline DWORD_PTR DedicatedAffinityMask() {
    const unsigned dedicated = DedicatedThreads();
    return (1ULL << dedicated) - 1ULL;
}

/// Affinity mask for Main Loop thread (Thread 0)
inline DWORD_PTR MainAffinityMask() {
    return 1ULL << 0;
}

/// Affinity mask for Render Submission thread (Thread 1)
inline DWORD_PTR RenderAffinityMask() {
    return 1ULL << (DedicatedThreads() >= 2 ? 1 : 0);
}

/// Affinity mask for Audio thread (Thread 2)
inline DWORD_PTR AudioAffinityMask() {
    return 1ULL << (DedicatedThreads() >= 3 ? 2 : 0);
}

/// Affinity mask for I/O and Asset Decompression thread (Thread 3)
inline DWORD_PTR IOAffinityMask() {
    return 1ULL << (DedicatedThreads() >= 4 ? 3 : 0);
}

/// Affinity mask for background worker threads (Threads 4..27 on 28t Xeon: 0x0FFFFFF0)
inline DWORD_PTR WorkerAffinityMask() {
    const unsigned total = Available();
    const DWORD_PTR dedicated = DedicatedAffinityMask();
    const DWORD_PTR total_mask = total >= 64 ? ~0ULL : ((1ULL << total) - 1ULL);
    const DWORD_PTR worker_mask = total_mask & ~dedicated;
    return worker_mask ? worker_mask : total_mask;
}

/// Configures the Render Thread: THREAD_PRIORITY_ABOVE_NORMAL and dedicated Render core affinity
inline void MakeRenderThread() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    DWORD_PTR mask = RenderAffinityMask();
    if (mask) {
        SetThreadAffinityMask(GetCurrentThread(), mask);
    }
}

/// Configures the Main Loop Thread
inline void MakeMainThread() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    DWORD_PTR mask = MainAffinityMask();
    if (mask) {
        SetThreadAffinityMask(GetCurrentThread(), mask);
    }
}

/// Configures the Audio Thread
inline void MakeAudioThread() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    DWORD_PTR mask = AudioAffinityMask();
    if (mask) {
        SetThreadAffinityMask(GetCurrentThread(), mask);
    }
}

/// Configures the I/O & Asset Thread
inline void MakeIOThread() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    DWORD_PTR mask = IOAffinityMask();
    if (mask) {
        SetThreadAffinityMask(GetCurrentThread(), mask);
    }
}

/// Configures Shader Worker threads:
/// - THREAD_PRIORITY_BELOW_NORMAL during gameplay, THREAD_PRIORITY_HIGHEST during warmup/loading
/// - WorkerAffinityMask() (isolated from dedicated cores 0..3)
inline void MakeShaderWorker(bool isLoadingOrWarmup = false) {
    SetThreadPriority(GetCurrentThread(), isLoadingOrWarmup ? THREAD_PRIORITY_HIGHEST : THREAD_PRIORITY_BELOW_NORMAL);
    DWORD_PTR mask = WorkerAffinityMask();
    if (mask) {
        SetThreadAffinityMask(GetCurrentThread(), mask);
    }
}

/// Dynamically adjusts shader worker priority
inline void SetThreadWarmupPriority(HANDLE thread, bool isLoadingOrWarmup) {
    if (thread) {
        SetThreadPriority(thread, isLoadingOrWarmup ? THREAD_PRIORITY_HIGHEST : THREAD_PRIORITY_BELOW_NORMAL);
    }
}

/// The calling thread only runs when cores are otherwise idle (lowest Windows priority).
inline void MakeBackground() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
}

#else

/// Hardware threads available to the process.
inline unsigned Available() {
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) == 0) {
        return std::max(1, CPU_COUNT(&set));
    }
    return std::max(1u, std::thread::hardware_concurrency());
}

inline unsigned DedicatedThreads() {
    const unsigned total = Available();
    if (total >= 16) return 4u;
    if (total >= 8)  return 2u;
    return 1u;
}

inline unsigned WorkerThreads() {
    if (const char* env = std::getenv("THEORDER_WORKER_THREADS")) {
        int custom = std::atoi(env);
        if (custom > 0) return static_cast<unsigned>(custom);
    }
    if (const char* xeon = std::getenv("THEORDER_XEON_MODE")) {
        if (std::strcmp(xeon, "0") == 0 || std::strcmp(xeon, "off") == 0) {
            return 4u;
        }
    }
    const unsigned total = Available();
    const unsigned dedicated = DedicatedThreads();
    return total > dedicated ? (total - dedicated) : 1u;
}

inline unsigned AsyncShaderWorkers() {
    return WorkerThreads();
}

inline void MakeRenderThread() {
    struct sched_param param{};
    param.sched_priority = 1;
    sched_setscheduler(0, SCHED_RR, &param);
}

inline void MakeMainThread() {}
inline void MakeAudioThread() {}
inline void MakeIOThread() {}

inline void MakeShaderWorker(bool isLoadingOrWarmup = false) {
    if (isLoadingOrWarmup) {
        setpriority(PRIO_PROCESS, static_cast<id_t>(gettid()), -5);
    } else {
        setpriority(PRIO_PROCESS, static_cast<id_t>(gettid()), 5);
    }
}

/// The calling thread only runs on otherwise idle cores (falls back to the lowest nice level).
inline void MakeBackground() {
    sched_param param{};
    if (sched_setscheduler(0, SCHED_IDLE, &param) != 0) {
        setpriority(PRIO_PROCESS, static_cast<id_t>(gettid()), 19);
    }
}

#endif

} // namespace PortThreads
