// SPDX-License-Identifier: GPL-2.0-or-later
// Class A regression (FEAT-002): the arena path of BufferCache::DownloadMemory must end any open
// dynamic-rendering instance before recording its pre-transfer vk::BufferMemoryBarrier2. Recording
// a pipeline barrier inside an open render pass is illegal without dynamicRenderingLocalRead
// (VUID-vkCmdPipelineBarrier2-None-09553 / -09554), and when the layer drops the barrier the
// producer/consumer ordering it guards is lost -> stale arena reads -> disappearing geometry/effects
// (#29/#30). We open an attachment-less render pass, drive the arena download, and assert the layer
// never flagged 09553/09554. Headless: skips cleanly (exit 0) when no Vulkan device is present.
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_runtime.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"
#undef NDEBUG
#include <cassert>

namespace {

// Incremented by the debug-utils callback below for the two VUIDs this regression guards.
std::atomic<int> g_renderpass_barrier_vuids{0};

VKAPI_ATTR VkBool32 VKAPI_CALL CountRenderPassBarrierVuids(
    vk::DebugUtilsMessageSeverityFlagBitsEXT, vk::DebugUtilsMessageTypeFlagsEXT,
    const vk::DebugUtilsMessengerCallbackDataEXT* data, void*) {
    if (data && data->pMessageIdName) {
        const std::string_view id{data->pMessageIdName};
        if (id.find("VUID-vkCmdPipelineBarrier2-None-09553") != std::string_view::npos ||
            id.find("VUID-vkCmdPipelineBarrier2-None-09554") != std::string_view::npos) {
            g_renderpass_barrier_vuids.fetch_add(1, std::memory_order_relaxed);
            std::fprintf(stderr, "renderpass-barrier: validation flagged %s\n", data->pMessageIdName);
        }
    }
    return VK_FALSE;
}

} // namespace

int main() {
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
#endif
    using namespace Vulkan;
    using namespace VideoCore;

    // Match the DMA barrier fixture's upload path (no deferred uploads) so the arena download
    // records synchronously.
    runtime_disabled_optimizations = PortToggle::DeferredUploads;

    // Validation on: the headless Instance installs the layer and the engine's own debug callback.
    // We add a second messenger whose only job is to count the two VUIDs we care about.
    Instance instance(s32{-1}, true);

    const vk::DebugUtilsMessengerCreateInfoEXT msg_ci = {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = CountRenderPassBarrierVuids,
    };
    auto [messenger_result, messenger] =
        instance.GetInstance().createDebugUtilsMessengerEXTUnique(msg_ci);
    if (messenger_result != vk::Result::eSuccess) {
        std::printf("renderpass-barrier: could not install debug messenger (%s); skipping\n",
                    vk::to_string(messenger_result).c_str());
        return 0;
    }

    Scheduler scheduler(instance);
    Runtime runtime(instance, scheduler);
    PageManager page_manager(nullptr);
    alignas(TextureCache) std::byte texture_storage[sizeof(TextureCache)];
    auto* textures = reinterpret_cast<TextureCache*>(texture_storage);
    // The caches hold references to each other, as they do in Rasterizer's construction order.
    BufferCache buffers(instance, scheduler, runtime, nullptr, *textures, page_manager);
    std::construct_at(textures, instance, scheduler, runtime, nullptr, buffers, page_manager);
    scheduler.SetSubmitCallback([&](SubmitInfo& info) {
        runtime.FlushBarriers();
        buffers.SubmitPendingArenaBinds(info);
    });

    constexpr VAddr address = 0x1000000;
    constexpr u32 size = 64_KB;
    page_manager.OnGpuMap(address, size);
    const auto [arena, offset] =
        buffers.ObtainBuffer(address, size, ObtainBufferFlags::IgnoreStreamBuffer);
    (void)arena;
    (void)offset;
    runtime.FlushBarriers();

    // Make the arena's sparse memory resident and bound (as a frame would before writing into it),
    // so the download's transfer copy reads a backed buffer. Mirrors the DMA barrier fixture.
    buffers.InvalidateMemory(address, size);
    buffers.SynchronizeDmaBuffers();
    runtime.FlushBarriers();
    scheduler.Finish();

    // The GPU wrote this range: a later CPU-read download must synchronize those writes with a
    // buffer barrier before the transfer read (the barrier whose placement this test guards).
    buffers.MarkRegionAsGpuModified(address, size);

    // Open an attachment-less dynamic-rendering instance, as a frame does mid-draw. Without the
    // fix the arena download records its pipelineBarrier2 inside this pass.
    RenderState state{};
    state.width = 1;
    state.height = 1;
    state.num_layers = 1;
    state.num_color_attachments = 0;
    scheduler.BeginRendering(state);

    // Drive the arena DownloadMemory path (ReadMemory -> flush_request -> DownloadMemory). The
    // caches hold no liverpool here, so run the flush synchronously (assume_locks = true).
    buffers.ReadMemory(address, size, /*is_write=*/false, /*async=*/false, /*assume_locks=*/true);

    // Flush recorded commands so the validation layer sees the barrier in context.
    scheduler.Finish();

    const int vuids = g_renderpass_barrier_vuids.load(std::memory_order_relaxed);
    assert(vuids == 0 && "arena DownloadMemory recorded a buffer barrier inside an open render pass "
                         "(VUID-vkCmdPipelineBarrier2-None-09553/09554): EndRendering is missing");

    std::destroy_at(textures);
    std::printf("renderpass-barrier: arena download barrier recorded outside the render pass "
                "(09553/09554 count = %d)\n",
                vuids);
    return 0;
}
