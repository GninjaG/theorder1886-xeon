// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>

#include "video_core/texture_cache/types.h"

namespace VideoCore {

struct ImageInfo;

[[nodiscard]] constexpr bool CanCopyColorAndDepth(u32 src_samples, u32 dst_samples,
                               bool dst_is_depth, u32 dst_guest_samples) {
    const bool guest_ms_depth = dst_is_depth && dst_guest_samples > 1;
    return (src_samples == 1 && dst_samples == 1 && !guest_ms_depth) ||
            (src_samples == 1 && (dst_samples > 1 || guest_ms_depth) && dst_is_depth) ||
            (src_samples > 1 && src_samples == dst_samples && dst_is_depth);
}

[[nodiscard]] std::optional<Extent3D> GetAliasCopyExtent(const ImageInfo& src,
                                                         const ImageInfo& dst);

} // namespace VideoCore
