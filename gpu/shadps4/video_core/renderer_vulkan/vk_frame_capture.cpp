// SPDX-License-Identifier: GPL-2.0-or-later
#include "video_core/renderer_vulkan/vk_frame_capture.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <mutex>

#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/image_info.h"
#include "port_toggles.h"

namespace Vulkan {

namespace {

struct Target {
    VAddr address{};
    vk::Format format{};
    u32 width{}, height{};
    u64 stable_id{}; ///< FEAT-001: Image::stable_id, 0 when unknown
    bool operator==(const Target&) const = default;
};

struct Entry {
    bool compute{};
    bool has_seq{}; ///< whether frame/seq below were set by the first draw/dispatch
    u64 frame{};    ///< (frame, seq) of the first draw/dispatch recorded into this entry
    u64 seq{};
    std::vector<Target> colors;
    Target depth;
    u32 draws{};
    u64 indices{};
    std::vector<u64> shaders; ///< vs/ps pairs (graphics) or cs (compute), first few
    std::vector<std::pair<Target, bool>> sampled; ///< textures (false) and storage images (true)
    std::string state; ///< census line of the first pipeline (vk_pass_census.h)
    std::string note;
    std::vector<std::string> buffers; ///< dumped constants, per draw
};

// GPU thread only.
std::vector<Entry> entries;
std::vector<std::pair<Target, bool>> pending_sampled;
std::vector<std::string> pending_buffers;

void AddBuffers(Entry& entry) {
    // Constants of full-screen passes (post-processing: camera matrices) and of the first draw
    // of each geometry pass.
    if (entry.draws <= 4) {
        for (auto& b : pending_buffers) {
            entry.buffers.push_back(std::move(b));
        }
    }
    pending_buffers.clear();
}
bool pass_open = false;
std::mutex display_mutex;
std::vector<VAddr> display_buffers;
// port: read on every draw; the game registers a handful of display buffers once.
std::array<std::atomic<VAddr>, 16> display_list{};
std::atomic<u32> display_count{0};

Target ToTarget(const VideoCore::ImageInfo& info, u64 stable_id = 0) {
    return {info.guest_address, info.pixel_format, info.size.width, info.size.height, stable_id};
}

std::string Describe(const Target& t) {
    if (!t.address) {
        return "-";
    }
    if (t.stable_id) {
        return std::format("img#{} {:#x} {} {}x{}", t.stable_id, t.address, vk::to_string(t.format),
                           t.width, t.height);
    }
    return std::format("{:#x} {} {}x{}", t.address, vk::to_string(t.format), t.width, t.height);
}

void AddSampled(Entry& entry) {
    for (const auto& s : pending_sampled) {
        if (entry.sampled.size() < 32 &&
            std::ranges::find(entry.sampled, s) == entry.sampled.end()) {
            entry.sampled.push_back(s);
        }
    }
    pending_sampled.clear();
}

void AddShader(Entry& entry, u64 hash) {
    if (entry.shaders.size() < 8 && std::ranges::find(entry.shaders, hash) == entry.shaders.end()) {
        entry.shaders.push_back(hash);
    }
}

std::mutex result_mutex;
std::string result; ///< for the menu (FrameCapture::LastResult)

void SetResult(std::string text) {
    std::printf("Frame capture: %s\n", text.c_str());
    std::scoped_lock lk{result_mutex};
    result = std::move(text);
}

// THEORDER_CAPTURE_DIR, else <user>/captures (THEORDER_GPU_USER_DIR, the saves folder).
std::filesystem::path CaptureDir() {
    if (const char* dir = std::getenv("THEORDER_CAPTURE_DIR"); dir && dir[0]) {
        return dir;
    }
    const char* user = std::getenv("THEORDER_GPU_USER_DIR");
    return std::filesystem::path(user && user[0] ? user : "user") / "captures";
}

void Write(VAddr presented) {
    const auto dir = CaptureDir();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    const std::time_t now = std::time(nullptr);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
    static u32 sequence = 0;
    const std::string path = (dir / std::format("frame_{}_{}.txt", stamp, ++sequence)).string();
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        std::printf("Frame capture: cannot write %s\n", path.c_str());
        SetResult("Capture failed: cannot write " + path);
        return;
    }
    // The flip the presenter prepared last, as frame dumps report it ("Frame dump N at flip M").
    std::fprintf(f, "presented buffer %#llx, flip %llu\n", static_cast<unsigned long long>(presented),
                 static_cast<unsigned long long>(PortStats::flips.load(std::memory_order_relaxed)));
    u32 index = 0;
    for (const auto& e : entries) {
        // FEAT-001: (frame,draw_seq) header so each entry links to the stable draw identity.
        char seq_hdr[48] = "";
        if (e.has_seq) {
            std::snprintf(seq_hdr, sizeof(seq_hdr), " (frame %llu seq %llu)",
                          static_cast<unsigned long long>(e.frame),
                          static_cast<unsigned long long>(e.seq));
        }
        if (e.compute) {
            std::fprintf(f, "\n#%u%s COMPUTE dispatches %u, cs", index++, seq_hdr, e.draws);
        } else {
            std::fprintf(f, "\n#%u%s PASS draws %u (indices %llu), vs/ps", index++, seq_hdr, e.draws,
                         static_cast<unsigned long long>(e.indices));
        }
        for (const u64 h : e.shaders) {
            std::fprintf(f, " %016llx", static_cast<unsigned long long>(h));
        }
        std::fprintf(f, "\n");
        for (size_t i = 0; i < e.colors.size(); ++i) {
            std::fprintf(f, "  color%zu %s\n", i, Describe(e.colors[i]).c_str());
        }
        if (!e.compute) {
            std::fprintf(f, "  depth  %s\n", Describe(e.depth).c_str());
        }
        if (!e.state.empty()) {
            std::fprintf(f, "  %s\n", e.state.c_str());
        }
        for (const auto& [t, storage] : e.sampled) {
            std::fprintf(f, "  %s %s\n", storage ? "writes " : "samples", Describe(t).c_str());
        }
        if (!e.note.empty()) {
            std::fprintf(f, "  note: %s\n", e.note.c_str());
        }
        for (const auto& b : e.buffers) {
            std::fprintf(f, "%s\n", b.c_str());
        }
    }
    std::fclose(f);
    std::printf("Frame capture: %zu passes written to %s\n", entries.size(), path.c_str());
    SetResult(std::format("Frame captured ({} passes): {}", entries.size(), path));
}

} // namespace

void FrameCapture::OnFlip(VAddr presented_address) {
    last_presented.store(presented_address, std::memory_order_relaxed);
    flips.fetch_add(1, std::memory_order_release);
    static const char* trigger = std::getenv("THEORDER_CAPTURE_TRIGGER");
    if (state.load(std::memory_order_relaxed) != Idle) {
        return;
    }
    bool arm = requested.exchange(false, std::memory_order_acq_rel);
    if (!arm && trigger && std::filesystem::exists(trigger)) {
        std::error_code ec;
        std::filesystem::remove(trigger, ec);
        arm = true;
    }
    if (arm) {
        state.store(Armed, std::memory_order_release);
        std::printf("Frame capture: armed\n");
    }
}

void FrameCapture::Request() {
    SetResult("Capturing the next frame...");
    requested.store(true, std::memory_order_release);
}

std::string FrameCapture::LastResult() {
    std::scoped_lock lk{result_mutex};
    return result;
}

void FrameCapture::Poll() {
    // Reset the per-frame draw/dispatch sequence once at each frame boundary (the flip counter
    // advancing). GPU thread; Poll runs at every draw/dispatch.
    static u64 last_frame = ~0ULL;
    const u64 frame = flips.load(std::memory_order_relaxed);
    if (frame != last_frame) {
        last_frame = frame;
        draw_seq.store(0, std::memory_order_relaxed);
        dispatch_seq.store(0, std::memory_order_relaxed);
    }
}

void FrameCapture::LogSkipStall(u64 frame, u64 draw_seq_value, u64 target_id, u64 vs_hash,
                                u64 ps_hash, DrawDisposition disposition) {
    std::printf("frame %llu draw %llu img#%llu vs/ps %016llx %016llx %s\n",
                static_cast<unsigned long long>(frame),
                static_cast<unsigned long long>(draw_seq_value),
                static_cast<unsigned long long>(target_id),
                static_cast<unsigned long long>(vs_hash),
                static_cast<unsigned long long>(ps_hash),
                disposition == DrawDisposition::Skipped ? "SKIPPED" : "STALLED");
}

bool FrameCapture::IsDisplayBuffer(VAddr address) {
    const u32 count = display_count.load(std::memory_order_acquire);
    for (u32 i = 0; i < count; ++i) {
        if (display_list[i].load(std::memory_order_relaxed) == address) {
            return true;
        }
    }
    if (count < display_list.size()) {
        return false;
    }
    std::scoped_lock lk{display_mutex};
    return std::ranges::find(display_buffers, address) != display_buffers.end();
}

void FrameCapture::AddDisplayBuffer(VAddr address) {
    std::scoped_lock lk{display_mutex};
    if (std::ranges::find(display_buffers, address) == display_buffers.end()) {
        display_buffers.push_back(address);
        const u32 count = display_count.load(std::memory_order_relaxed);
        if (count < display_list.size()) {
            display_list[count].store(address, std::memory_order_relaxed);
            display_count.store(count + 1, std::memory_order_release);
        }
    }
}

void FrameCapture::BeginPass(const VideoCore::ImageInfo* const* colors, const u64* color_ids,
                             u32 num_colors, const VideoCore::ImageInfo* depth, u64 depth_id,
                             std::string census) {
    std::vector<Target> targets;
    bool display = false;
    {
        std::scoped_lock lk{display_mutex};
        for (u32 i = 0; i < num_colors; ++i) {
            targets.push_back(colors[i] ? ToTarget(*colors[i], color_ids ? color_ids[i] : 0)
                                        : Target{});
            display |= colors[i] && std::ranges::find(display_buffers, colors[i]->guest_address) !=
                                        display_buffers.end();
        }
    }
    // Frame boundaries in the command stream: the pass that writes a display buffer as its only
    // target. port: God of War III also draws scene passes (two targets) into display buffers.
    display = display && targets.size() == 1;
    const bool new_pass = !(pass_open && !entries.empty() && !entries.back().compute &&
                            entries.back().colors == targets);
    if (display && new_pass) {
        if (state.load(std::memory_order_relaxed) == Recording) {
            Write(last_presented.load(std::memory_order_relaxed));
            entries.clear();
            pending_sampled.clear();
            pending_buffers.clear();
            state.store(Idle, std::memory_order_release);
            return;
        }
        entries.clear();
        pending_sampled.clear();
        pending_buffers.clear();
        pass_open = false;
        state.store(Recording, std::memory_order_release);
    }
    if (state.load(std::memory_order_relaxed) != Recording) {
        pending_sampled.clear();
        pending_buffers.clear();
        return;
    }
    const Target d = depth ? ToTarget(*depth, depth_id) : Target{};
    if (pass_open && !entries.empty() && !entries.back().compute &&
        entries.back().colors == targets && entries.back().depth == d) {
        return;
    }
    entries.push_back(
        {.compute = false, .colors = std::move(targets), .depth = d, .state = std::move(census)});
    pass_open = true;
}

void FrameCapture::Draw(u64 frame, u64 draw_seq, u64 vs_hash, u64 ps_hash, u32 num_indices,
                        u32 num_instances) {
    if (state.load(std::memory_order_relaxed) != Recording || entries.empty() ||
        entries.back().compute) {
        return;
    }
    auto& e = entries.back();
    if (!e.has_seq) {
        e.has_seq = true;
        e.frame = frame;
        e.seq = draw_seq;
    }
    ++e.draws;
    e.indices += u64(num_indices) * std::max(num_instances, 1u);
    AddShader(e, vs_hash);
    AddShader(e, ps_hash);
    AddSampled(e);
    AddBuffers(e);
}

void FrameCapture::Dispatch(u64 frame, u64 dispatch_seq, u64 cs_hash, u32 x, u32 y, u32 z) {
    if (state.load(std::memory_order_relaxed) != Recording) {
        pending_sampled.clear();
        pending_buffers.clear();
        return;
    }
    if (!entries.empty() && entries.back().compute && !entries.back().shaders.empty() &&
        entries.back().shaders[0] == cs_hash) {
        ++entries.back().draws;
    } else {
        entries.push_back({.compute = true,
                           .has_seq = true,
                           .frame = frame,
                           .seq = dispatch_seq,
                           .draws = 1,
                           .shaders = {cs_hash}});
        entries.back().note = std::format("groups {}x{}x{}", x, y, z);
    }
    AddSampled(entries.back());
    AddBuffers(entries.back());
    pass_open = false;
}

void FrameCapture::Buffer(u64 stage_hash, u32 slot, VAddr address, const void* data, u64 size) {
    if (!data) {
        return;
    }
    const u64 bytes = std::min<u64>(size, 1024) & ~u64(3);
    std::string out = std::format("  buffer stage {:016x} slot {} at {:#x} size {}:", stage_hash,
                                  slot, address, size);
    const auto* words = static_cast<const float*>(data);
    for (u64 i = 0; i < bytes / 4; ++i) {
        if (i % 8 == 0) {
            out += std::format("\n    [{:3}]", i);
        }
        out += std::format(" {:12.6g}", words[i]);
    }
    pending_buffers.push_back(std::move(out));
}

void FrameCapture::Sampled(const VideoCore::ImageInfo& info, u64 stable_id, bool storage) {
    if (pending_sampled.size() < 64) {
        pending_sampled.emplace_back(ToTarget(info, stable_id), storage);
    }
}

void FrameCapture::Note(const char* text) {
    if (!entries.empty()) {
        entries.back().note += text;
    }
}

} // namespace Vulkan
