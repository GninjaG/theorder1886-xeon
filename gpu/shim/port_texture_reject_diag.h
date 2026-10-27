// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Opt-in diagnostic sampler for texture bindings rejected by the Vulkan rasterizer.
// The sampler is intentionally dependency-free so its behavior is unit tested without a GPU.

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace Vulkan::TextureRejectDiag {

enum class Reason : std::uint8_t {
    LowAddress,
    InvalidDataFormat,
    FmaskDescriptor,
    ExcessiveLayers,
    InvalidArrayRange,
    InvalidMipRange,
    ComputeFallbackMismatch,
    UnmappedAddress,
    UnsupportedFormat,
    ExcessiveDimensions,
    ExcessiveTexels,
    ExcessiveLayerDimensions,
    ImplausibleImage,
    Count,
};

inline constexpr std::size_t ReasonCount = static_cast<std::size_t>(Reason::Count);

constexpr const char* Name(Reason reason) noexcept {
    switch (reason) {
    case Reason::LowAddress: return "low-address";
    case Reason::InvalidDataFormat: return "invalid-data-format";
    case Reason::FmaskDescriptor: return "fmask-special";
    case Reason::ExcessiveLayers: return "excessive-layers";
    case Reason::InvalidArrayRange: return "invalid-array-range";
    case Reason::InvalidMipRange: return "invalid-mip-range";
    case Reason::ComputeFallbackMismatch: return "compute-fallback-mismatch";
    case Reason::UnmappedAddress: return "unmapped-address";
    case Reason::UnsupportedFormat: return "unsupported-vulkan-format";
    case Reason::ExcessiveDimensions: return "excessive-dimensions";
    case Reason::ExcessiveTexels: return "excessive-texels";
    case Reason::ExcessiveLayerDimensions: return "excessive-layer-dimensions";
    case Reason::ImplausibleImage: return "implausible-guest-image";
    case Reason::Count: return "invalid-reason";
    }
    return "invalid-reason";
}

// Pure precedence rules, separately verified with synthetic descriptor flags.
// Called only after the original renderer decided a texture must bind as null.
constexpr Reason EarlyReason(bool low_address, bool invalid_format, bool is_fmask,
                             bool excessive_layers, bool invalid_array,
                             bool invalid_mip) noexcept {
    if (low_address) return Reason::LowAddress;
    if (invalid_format) return Reason::InvalidDataFormat;
    if (is_fmask) return Reason::FmaskDescriptor;
    if (excessive_layers) return Reason::ExcessiveLayers;
    if (invalid_array) return Reason::InvalidArrayRange;
    return invalid_mip ? Reason::InvalidMipRange : Reason::Count;
}

constexpr Reason MappingReason(bool unmapped, bool unsupported_format,
                               bool oversized_dimensions, bool excessive_layers,
                               bool excessive_texels, bool excessive_layer_dimensions) noexcept {
    if (unmapped) return Reason::UnmappedAddress;
    if (unsupported_format) return Reason::UnsupportedFormat;
    if (oversized_dimensions) return Reason::ExcessiveDimensions;
    if (excessive_layers) return Reason::ExcessiveLayers;
    if (excessive_texels) return Reason::ExcessiveTexels;
    return excessive_layer_dimensions ? Reason::ExcessiveLayerDimensions : Reason::Count;
}

constexpr bool ShouldLog(std::uint64_t count) noexcept {
    // First eight, then exponentially sparse. 8 + log2(count) per reason,
    // rather than thousands of logs per frame during broken rendering.
    return count > 0 && (count <= 8 || (count & (count - 1)) == 0);
}

class Sampler {
public:
    // 0 means no log; otherwise returns the occurrence number.
    // A caller may restrict the data to a particular PS4 shader hash.
    std::uint64_t Observe(Reason reason, std::uint64_t shader_hash, bool enabled,
                          std::uint64_t shader_filter = 0) noexcept {
        const auto index = static_cast<std::size_t>(reason);
        if (!enabled || index >= ReasonCount ||
            (shader_filter != 0 && shader_filter != shader_hash)) {
            return 0;
        }
        const auto count = counts_[index].fetch_add(1, std::memory_order_relaxed) + 1;
        return ShouldLog(count) ? count : 0;
    }

    std::uint64_t Count(Reason reason) const noexcept {
        const auto index = static_cast<std::size_t>(reason);
        return index < ReasonCount ? counts_[index].load(std::memory_order_relaxed) : 0;
    }

private:
    std::array<std::atomic<std::uint64_t>, ReasonCount> counts_{};
};

inline bool Enabled() noexcept {
    static const bool enabled = [] {
        const char* value = std::getenv("THEORDER_DIAG_TEXTURE_REJECTS");
        return value && value[0] == '1' && value[1] == '\0';
    }();
    return enabled;
}

inline std::uint64_t ShaderFilter() noexcept {
    static const std::uint64_t filter = [] {
        const char* value = std::getenv("THEORDER_DIAG_TEXTURE_SHADER");
        if (!value || !*value) return std::uint64_t{0};
        char* end = nullptr;
        const auto parsed = std::strtoull(value, &end, 0);
        return (end && *end == '\0') ? static_cast<std::uint64_t>(parsed) : 0;
    }();
    return filter;
}

inline std::uint64_t Record(Reason reason, std::uint64_t shader_hash) noexcept {
    if (!Enabled()) return 0;
    static Sampler sampler;
    return sampler.Observe(reason, shader_hash, true, ShaderFilter());
}

} // namespace Vulkan::TextureRejectDiag
