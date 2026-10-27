// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

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
} // namespace Shader
