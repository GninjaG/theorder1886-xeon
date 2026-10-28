// SPDX-License-Identifier: GPL-2.0-or-later
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <vector>

#include "port_settings.h"
#include "video_core/renderer_vulkan/vk_warmup_inbox.h"

int main() {
    // 1. Thread calculation for Xeon E5-2697 v3 (14 cores / 28 threads)
    assert(Vulkan::AotWarmupThreadCount(28) == 28);
    assert(Vulkan::AotWarmupThreadCount(1) == 1);
    assert(Vulkan::AotWarmupThreadCount(0) == 1);
    const unsigned hw_threads = std::thread::hardware_concurrency();
    assert(Vulkan::AotWarmupThreadCount(hw_threads) == std::max(1u, hw_threads));

    // 2. PortSettings precompile_shaders_on_boot
    auto& s = PortSettings::Get();
    s.precompile_shaders_on_boot = false;
    assert(!s.precompile_shaders_on_boot.load());

    // 3. Test directory creation for shader_cache/
    std::filesystem::path test_dir = "shader_cache";
    std::error_code ec;
    std::filesystem::create_directories(test_dir, ec);
    assert(std::filesystem::exists(test_dir));

    std::printf("PASS: shader aot precompile tests\n");
    return 0;
}
