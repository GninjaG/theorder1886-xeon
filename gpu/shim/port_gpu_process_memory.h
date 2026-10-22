// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <cmath>
#include <span>
#include <string>
#include <vector>

namespace PortPerfStats {

// WDDM process counters, summed across physical adapters, not adapter budgets.
inline double ProcessCounterBytes(std::span<const PDH_FMT_COUNTERVALUE_ITEM_W> items,
                                  std::wstring_view pid) {
    double bytes = 0;
    bool found = false;
    for (const auto& item : items) {
        if (!item.szName || !std::wstring_view{item.szName}.starts_with(pid)) {
            continue;
        }
        const auto& value = item.FmtValue;
        if ((value.CStatus != PDH_CSTATUS_VALID_DATA && value.CStatus != PDH_CSTATUS_NEW_DATA) ||
            !std::isfinite(value.doubleValue) || value.doubleValue < 0) {
            return -1;
        }
        bytes += value.doubleValue;
        found = true;
    }
    return found ? bytes : -1;
}

class GpuProcessMemory {
public:
    GpuProcessMemory() : pid{L"pid_" + std::to_wstring(GetCurrentProcessId()) + L"_"} {
        if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS) {
            query = nullptr;
            return;
        }
        Add(L"\\GPU Process Memory(*)\\Dedicated Usage", dedicated);
        Add(L"\\GPU Process Memory(*)\\Shared Usage", shared);
        PdhCollectQueryData(query);
    }

    ~GpuProcessMemory() {
        if (query) PdhCloseQuery(query);
    }
    GpuProcessMemory(const GpuProcessMemory&) = delete;
    GpuProcessMemory& operator=(const GpuProcessMemory&) = delete;

    struct Usage { double dedicated = -1, shared = -1; };
    Usage Get() {
        if (!query || PdhCollectQueryData(query) != ERROR_SUCCESS) return {};
        return {Read(dedicated), Read(shared)};
    }

private:
    void Add(const wchar_t* path, PDH_HCOUNTER& counter) {
        if (PdhAddEnglishCounterW(query, path, 0, &counter) != ERROR_SUCCESS) counter = nullptr;
    }
    double Read(PDH_HCOUNTER counter) {
        if (!counter) return -1;
        DWORD size = 0, count = 0;
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, nullptr) !=
            PDH_MORE_DATA) return -1;
        buffer.resize((size + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W) - 1) /
                      sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, buffer.data()) !=
            ERROR_SUCCESS) return -1;
        return ProcessCounterBytes({buffer.data(), count}, pid);
    }

    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER dedicated = nullptr, shared = nullptr;
    std::wstring pid;
    std::vector<PDH_FMT_COUNTERVALUE_ITEM_W> buffer;
};
} // namespace PortPerfStats
#endif
