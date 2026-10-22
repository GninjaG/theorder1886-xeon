// SPDX-License-Identifier: GPL-2.0-or-later
#include "port_perf_stats.h"

#include <chrono>
#include <cstdlib>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>
#include "port_process_memory.h"
#include "port_gpu_process_memory.h"
#include "video_core/renderer_vulkan/vk_instance.h"

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#endif

namespace PortPerfStats {

namespace {

std::mutex mutex;
Sample latest;
std::once_flag started;

#ifdef _WIN32
uint64_t Ticks(const FILETIME& time) {
    return (uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime;
}

// This process's use of the GPU's 3D engines, as Task Manager shows it ("GPU Engine" counters,
// any vendor). Negative until the second sample or without the counters.
class GpuUsage {
public:
    GpuUsage() {
        if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS) {
            query = nullptr;
            return;
        }
        if (PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) !=
            ERROR_SUCCESS) {
            PdhCloseQuery(query);
            query = nullptr;
            return;
        }
        PdhCollectQueryData(query);
        pid = L"pid_" + std::to_wstring(GetCurrentProcessId()) + L"_";
    }

    float Percent() {
        if (!query || PdhCollectQueryData(query) != ERROR_SUCCESS) {
            return -1.0f;
        }
        DWORD size = 0, count = 0;
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count,
                                         nullptr) != PDH_MORE_DATA) {
            return -1.0f;
        }
        buffer.resize(size);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count,
                                         items) != ERROR_SUCCESS) {
            return -1.0f;
        }
        double total = 0.0;
        bool found = false;
        for (DWORD i = 0; i < count; ++i) {
            const wchar_t* name = items[i].szName;
            if ((items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA ||
                 items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA) && wcsstr(name, pid.c_str()) &&
                wcsstr(name, L"engtype_3D")) {
                total += items[i].FmtValue.doubleValue;
                found = true;
            }
        }
        return found ? std::clamp(float(total), 0.0f, 100.0f) : -1.0f;
    }

private:
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER counter = nullptr;
    std::wstring pid;
    std::vector<unsigned char> buffer;
};

void Run(const Vulkan::Instance* instance) {
    GpuUsage gpu;
    GpuProcessMemory gpu_memory;
    FILETIME created, ended, kernel, user;
    uint64_t last_process = 0, last_wall = 0;
    const unsigned cores = std::max(1u, std::thread::hardware_concurrency());
    constexpr double GiB = 1024.0 * 1024.0 * 1024.0;
    const char* perf_log = std::getenv("THEORDER_PERF_LOG");
    const bool log = perf_log && std::string_view{perf_log} == "1";
    auto last_log = std::chrono::steady_clock::now() - std::chrono::seconds(5);
    while (true) {
        Sample s;
        s.gpu_name = std::string(instance->GetModelName());
        const auto wall = std::chrono::steady_clock::now().time_since_epoch();
        const uint64_t wall_ticks =
            std::chrono::duration_cast<std::chrono::nanoseconds>(wall).count() / 100;
        if (GetProcessTimes(GetCurrentProcess(), &created, &ended, &kernel, &user)) {
            const uint64_t process = Ticks(kernel) + Ticks(user);
            if (last_wall) {
                s.cpu_percent = CpuPercent(process - last_process, wall_ticks - last_wall, cores);
            }
            last_process = process;
            last_wall = wall_ticks;
        }
        const auto memory = PortProcessMemory::Get();
        if (memory.private_available) {
            s.ram_used_gb = double(memory.private_bytes) / GiB;
        }
        if (memory.resident_available) {
            s.working_set_gb = double(memory.working_set) / GiB;
            s.peak_working_set_gb = double(memory.peak_working_set) / GiB;
        }
        s.renderer_allocated_gb = double(instance->GetRendererAllocationBytes()) / GiB;
        if (instance->CanReportMemoryUsage()) {
            s.vram_used_gb = double(instance->GetDeviceMemoryUsage()) / GiB;
            s.vram_total_gb = double(instance->GetDeviceMemoryBudgetNow()) / GiB;
        }
        s.gpu_percent = gpu.Percent();
        const auto gpu_bytes = gpu_memory.Get();
        if (gpu_bytes.dedicated >= 0) s.gpu_dedicated_gb = gpu_bytes.dedicated / GiB;
        if (gpu_bytes.shared >= 0) s.gpu_shared_gb = gpu_bytes.shared / GiB;
        const auto now = std::chrono::steady_clock::now();
        if (log && now - last_log >= std::chrono::seconds(5)) {
            const auto mib = [](double gib) { return gib < 0 ? -1.0 : gib * 1024.0; };
            std::printf("Process metrics: CPU %.2f%%; private %.2f MiB; working set %.2f MiB; "
                        "peak working set %.2f MiB; Vulkan allocations %.2f MiB; "
                        "GPU adapter %.2f MiB; adapter budget %.2f MiB; "
                        "GPU dedicated process %.2f MiB; GPU shared process %.2f MiB\n",
                        s.cpu_percent, mib(s.ram_used_gb), mib(s.working_set_gb),
                        mib(s.peak_working_set_gb), mib(s.renderer_allocated_gb),
                        mib(s.vram_used_gb), mib(s.vram_total_gb),
                        mib(s.gpu_dedicated_gb), mib(s.gpu_shared_gb));
            last_log = now;
        }
        {
            std::scoped_lock lock{mutex};
            latest = std::move(s);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
#else
void Run(const Vulkan::Instance* instance) {
    while (true) {
        Sample s;
        s.gpu_name = std::string(instance->GetModelName());
        {
            std::scoped_lock lock{mutex};
            latest = std::move(s);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
#endif

} // namespace

void Start(const Vulkan::Instance& instance) {
    // ponytail: never joined; a thread that ended while the game ran crashed in ntdll
    // (vk_pipeline_serialization.cpp, StopWarmUp). It sleeps between samples.
    std::call_once(started, [&] { std::thread(Run, &instance).detach(); });
}

Sample Latest() {
    std::scoped_lock lock{mutex};
    return latest;
}

} // namespace PortPerfStats
