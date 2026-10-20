// SPDX-License-Identifier: GPL-2.0-or-later
// Class A regression (FEAT-002): the arena path of BufferCache::DownloadMemory must end any open
// dynamic-rendering instance before recording its pre-transfer vk::BufferMemoryBarrier2. Recording
// a pipeline barrier inside an open render pass is illegal without dynamicRenderingLocalRead
// (VUID-vkCmdPipelineBarrier2-None-09553 / -09554), and when the layer drops the barrier the
// producer/consumer ordering it guards is lost -> stale arena reads -> disappearing geometry/effects
// (#29/#30). We open an attachment-less render pass, drive the arena download, and assert the layer
// never flagged 09553/09554. Headless: skips cleanly (exit 0) when no Vulkan device is present.
//
// Class B regression (FEAT-003): a depth image transitioned OUT of a depth-attachment layout must
// have its barrier src scope promoted to the depth store's LATE/EARLY_FRAGMENT_TESTS +
// DEPTH_STENCIL_ATTACHMENT_WRITE, otherwise the layout transition (itself a write) does not wait on
// the prior depth store -> SYNC-HAZARD-WRITE-AFTER-WRITE -> unstable depth -> shadow/Z flicker
// (#29/#30). We perform a real depth-attachment store, model the engine's under-scoped tracked
// state, transition the depth image to shader-read via Image::GetBarriers, and assert sync
// validation reported no WRITE-AFTER-WRITE on that depth image. This assertion is red before the
// GetBarriers src-scope promotion and green after.
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include "video_core/amdgpu/resource.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_runtime.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/image.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/texture_cache.h"
#undef NDEBUG
#include <cassert>

namespace {

// Incremented by the debug-utils callback below for the two Class A VUIDs this regression guards.
std::atomic<int> g_renderpass_barrier_vuids{0};
// Incremented for Class B: SYNC-HAZARD-WRITE-AFTER-WRITE messages (depth store vs layout transition).
std::atomic<int> g_depth_waw_hazards{0};

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
        if (id.find("SYNC-HAZARD-WRITE-AFTER-WRITE") != std::string_view::npos) {
            g_depth_waw_hazards.fetch_add(1, std::memory_order_relaxed);
            std::fprintf(stderr, "renderpass-barrier: sync validation flagged %s\n",
                         data->pMessageIdName);
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

    // Class B needs synchronization validation (SYNC-HAZARD-WRITE-AFTER-WRITE). The validation
    // layer reads THEORDER_VK_VALIDATION_SYNC once (cached) when the Instance is created, so it
    // must be set before construction. THEORDER_VK_VALIDATION keeps the core layer on too.
#ifdef _WIN32
    _putenv_s("THEORDER_VK_VALIDATION", "1");
    _putenv_s("THEORDER_VK_VALIDATION_SYNC", "1");
#else
    setenv("THEORDER_VK_VALIDATION", "1", 1);
    setenv("THEORDER_VK_VALIDATION_SYNC", "1", 1);
#endif

    // Validation on: the headless Instance installs the layer and the engine's own debug callback.
    // We add a second messenger whose only job is to count the VUIDs/hazards we care about.
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

    // --- Class B (FEAT-003): depth store vs layout-transition WRITE-AFTER-WRITE ---------------
    // Build a real depth target, perform an actual depth-attachment store into it, then transition
    // it to shader-read through Image::GetBarriers. The engine leaves the tracked backing->state of
    // a stored depth target in a depth-attachment layout with the depth-write access but an
    // under-scoped source stage (it does not carry the LATE/EARLY_FRAGMENT_TESTS of the store), so
    // the generated transition does not wait on the store unless GetBarriers promotes the src scope.
    {
        Common::SlotVector<ImageView> depth_views;
        ImageInfo depth_info{};
        depth_info.props.is_depth = 1;
        depth_info.pixel_format = vk::Format::eD32Sfloat;
        depth_info.type = AmdGpu::ImageType::Color2D;
        depth_info.resources = SubresourceExtent{1, 1};
        depth_info.size = Extent3D{64, 64, 1};
        depth_info.num_samples = 1;
        depth_info.guest_samples = 1;
        depth_info.guest_address = 0x104f080000ULL; // the repro's depth address, for the debug name
        depth_info.guest_size = 64 * 64 * 4;

        Image depth(instance, runtime, depth_views, depth_info);
        assert(depth.GetImage() && "depth image creation failed");

        // A depth-only image view for the attachment store.
        const vk::ImageViewCreateInfo view_ci{
            .image = depth.GetImage(),
            .viewType = vk::ImageViewType::e2D,
            .format = depth.backing->image.image_ci.format,
            .subresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eDepth,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        auto [view_result, depth_view] = instance.GetDevice().createImageViewUnique(view_ci);
        assert(view_result == vk::Result::eSuccess && "depth view creation failed");

        // 1) Real depth-attachment store: UNDEFINED -> depth-attachment, then a render pass whose
        //    depth attachment stores (loadOp=Clear, storeOp=Store). This is a genuine
        //    DEPTH_STENCIL_ATTACHMENT_WRITE at LATE_FRAGMENT_TESTS that sync validation tracks.
        runtime.Transit(&depth, vk::ImageLayout::eDepthAttachmentOptimal,
                        vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                            vk::PipelineStageFlagBits2::eLateFragmentTests,
                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite);
        runtime.FlushBarriers();
        scheduler.Record([view = *depth_view, image = depth.GetImage()](vk::CommandBuffer cmd) {
            const vk::RenderingAttachmentInfo depth_attachment{
                .imageView = view,
                .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = vk::ClearValue{.depthStencil = vk::ClearDepthStencilValue{1.0f, 0}},
            };
            cmd.beginRendering(vk::RenderingInfo{
                .renderArea = vk::Rect2D{{0, 0}, {64, 64}},
                .layerCount = 1,
                .colorAttachmentCount = 0,
                .pDepthAttachment = &depth_attachment,
            });
            cmd.endRendering();
        });

        // 2) Model the engine's under-scoped tracking: the stored depth target is left in a
        //    depth-attachment layout with the depth-write access, but a source stage that does NOT
        //    cover the fragment-test store (the Class B bug). Without the GetBarriers promotion the
        //    next transition inherits this stage and fails to wait on the store above.
        depth.backing->state.layout = vk::ImageLayout::eDepthAttachmentOptimal;
        depth.backing->state.access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        depth.backing->state.pl_stage = vk::PipelineStageFlagBits2::eTopOfPipe;

        // 3) Transition the depth image to shader-read via Image::GetBarriers. The layout change is
        //    itself a write; it must wait on the store. GetBarriers' depth src-scope promotion adds
        //    the fragment-test stage so sync validation sees a correctly ordered WRITE-AFTER-WRITE.
        runtime.Transit(&depth, vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::PipelineStageFlagBits2::eFragmentShader,
                        vk::AccessFlagBits2::eShaderSampledRead);
        runtime.FlushBarriers();
        scheduler.Finish();

        const int waw = g_depth_waw_hazards.load(std::memory_order_relaxed);
        assert(waw == 0 &&
               "depth layout transition did not wait on the depth store "
               "(SYNC-HAZARD-WRITE-AFTER-WRITE): Image::GetBarriers must promote the depth-write "
               "stage/access into the transition's src scope");
        std::printf("renderpass-barrier: depth transition ordered after the depth store "
                    "(WRITE-AFTER-WRITE count = %d)\n",
                    waw);
    }

    std::destroy_at(textures);
    std::printf("renderpass-barrier: arena download barrier recorded outside the render pass "
                "(09553/09554 count = %d)\n",
                vuids);
    return 0;
}
