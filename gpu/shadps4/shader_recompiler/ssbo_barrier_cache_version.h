// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

// Preserve warm SPIR-V for unaffected shaders. Cached Shader::Info contains
// both resource flags and is available before deciding whether to retranslate.
namespace Shader {
template <typename BufferRange>
constexpr bool NeedsFormattedSsboBarrierCacheVersion(const BufferRange& buffers) noexcept {
    for (const auto& buffer : buffers) {
        if (buffer.is_formatted && buffer.is_written) return true;
    }
    return false;
}

template <typename BufferRange>
constexpr bool NeedsWrittenSsboBarrierCacheVersion(const BufferRange& buffers) noexcept {
    // Cached resources mark atomics as written, but cannot distinguish them
    // from ordinary SSBO stores. Recompile this safe subset, not all shaders.
    for (const auto& buffer : buffers) {
        if (buffer.is_written) return true;
    }
    return false;
}

template <typename BufferRange>
constexpr std::uint32_t SsboBarrierCacheSalt(bool needs_ssbo_barriers,
                                             const BufferRange& buffers) noexcept {
    if (!needs_ssbo_barriers) return 0;
    return (NeedsFormattedSsboBarrierCacheVersion(buffers) ? (1u << 18) : 0u) |
           (NeedsWrittenSsboBarrierCacheVersion(buffers) ? (1u << 19) : 0u);
}
} // namespace Shader
