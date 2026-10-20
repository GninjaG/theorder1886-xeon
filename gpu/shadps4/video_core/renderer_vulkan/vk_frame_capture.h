// SPDX-License-Identifier: GPL-2.0-or-later
// port: frame analyzer. Creating the file named by THEORDER_CAPTURE_TRIGGER records the next full
// frame: every render pass (targets, depth, draw count, shaders, sampled textures) and compute
// dispatch, in order, plus the buffer that was presented. Input for placing a temporal
// upscaler (scene color, depth, motion vectors, UI) in the game's frame.

#pragma once

#include <atomic>
#include <cstdio>
#include <string>
#include <vector>

#include "common/types.h"

namespace VideoCore {
struct ImageInfo;
}

namespace Vulkan {

class FrameCapture {
public:
    /// VideoOut thread, per flip: frame boundary, presented buffer, trigger file check.
    static void OnFlip(VAddr presented_address);

    /// VideoOut: a display buffer the game registered. A pass drawing into one starts a frame
    /// in the GPU command stream (the flip itself runs a frame behind).
    static void AddDisplayBuffer(VAddr address);
    static bool IsDisplayBuffer(VAddr address);

    /// GPU thread: whether passes are being looked at (armed or recording; cheap).
    static bool Active() {
        return state.load(std::memory_order_relaxed) != Idle;
    }
    /// GPU thread: whether a frame is being recorded right now.
    static bool IsRecording() {
        return state.load(std::memory_order_relaxed) == Recording;
    }
    /// GPU thread, at every draw/dispatch: starts or ends a recording at frame boundaries.
    static void Poll();

    /// Stable per-frame index (the flip counter), for stable image/draw IDs. Any thread.
    static u64 CurrentFrameIndex() {
        return flips.load(std::memory_order_relaxed);
    }
    /// GPU thread: mint the next 0-based draw sequence within the current frame.
    static u64 NextDrawSeq() {
        return draw_seq.fetch_add(1, std::memory_order_relaxed);
    }
    /// GPU thread: mint the next 0-based dispatch sequence within the current frame.
    static u64 NextDispatchSeq() {
        return dispatch_seq.fetch_add(1, std::memory_order_relaxed);
    }
    /// Diagnostic log line that FEAT-004 reuses: a skipped or stalled draw and why.
    enum class DrawDisposition { Skipped, Stalled };
    static void LogSkipStall(u64 frame, u64 draw_seq, u64 target_id, u64 vs_hash, u64 ps_hash,
                             DrawDisposition disposition);

    /// `state`: the pass's census line (vk_pass_census.h), kept with the first pipeline that
    /// opened the pass.
    static void BeginPass(const VideoCore::ImageInfo* const* colors, const u64* color_ids,
                          u32 num_colors, const VideoCore::ImageInfo* depth, u64 depth_id,
                          std::string census);
    static void Draw(u64 frame, u64 draw_seq, u64 vs_hash, u64 ps_hash, u32 num_indices,
                     u32 num_instances);
    static void Dispatch(u64 frame, u64 dispatch_seq, u64 cs_hash, u32 x, u32 y, u32 z);
    static void Sampled(const VideoCore::ImageInfo& info, u64 stable_id, bool storage);
    /// Contents of a bound buffer (first 1 KiB), kept for small passes and a pass's first draw.
    static void Buffer(u64 stage_hash, u32 slot, VAddr address, const void* data, u64 size);
    static void Note(const char* text);

    /// In-game menu: records the next full frame (as the trigger file does).
    static void Request();
    /// What the last request did, for the menu: empty, "Capturing...", the file or an error.
    static std::string LastResult();

private:
    enum : u32 { Idle, Armed, Recording };
    static inline std::atomic<u32> state{Idle};
    static inline std::atomic<u64> flips{0};
    /// Per-frame draw/dispatch sequence, reset to 0 at each frame boundary in Poll().
    static inline std::atomic<u64> draw_seq{0};
    static inline std::atomic<u64> dispatch_seq{0};
    static inline std::atomic<VAddr> last_presented{0};
    static inline std::atomic<bool> requested{false};
};

} // namespace Vulkan
