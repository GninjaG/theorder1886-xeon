// SPDX-License-Identifier: GPL-2.0-or-later
// Host-only Vulkan swapchain policy regression: no Vulkan device or game files.
#include <array>
#include <cassert>
#include <cstdio>
#include "video_core/renderer_vulkan/vk_swapchain_policy.h"

using Alpha = vk::CompositeAlphaFlagBitsKHR;
using Flags = vk::CompositeAlphaFlagsKHR;
using Vulkan::ChooseCompositeAlpha;

static_assert(!ChooseCompositeAlpha(Flags{}).has_value());
static_assert(ChooseCompositeAlpha(Alpha::eOpaque) == Alpha::eOpaque);
static_assert(ChooseCompositeAlpha(Alpha::ePreMultiplied) == Alpha::ePreMultiplied);
static_assert(ChooseCompositeAlpha(Alpha::ePostMultiplied) == Alpha::ePostMultiplied);
static_assert(ChooseCompositeAlpha(Alpha::eInherit) == Alpha::eInherit);
// Opaque takes priority if supported. Inherit is *not* a safe default.
static_assert(ChooseCompositeAlpha(Alpha::eOpaque | Alpha::ePreMultiplied) ==
              Alpha::eOpaque);
static_assert(ChooseCompositeAlpha(Alpha::ePreMultiplied | Alpha::ePostMultiplied) ==
              Alpha::ePreMultiplied);
static_assert(ChooseCompositeAlpha(Alpha::ePostMultiplied | Alpha::eInherit) ==
              Alpha::eInherit);
static_assert(ChooseCompositeAlpha(Alpha::ePreMultiplied | Alpha::eInherit) ==
              Alpha::eInherit);

int main() {
    constexpr std::array<Flags, 7> supported = {
        Alpha::eOpaque,
        Alpha::ePreMultiplied,
        Alpha::ePostMultiplied,
        Alpha::eInherit,
        Alpha::eOpaque | Alpha::eInherit,
        Alpha::ePreMultiplied | Alpha::ePostMultiplied,
        Alpha::ePostMultiplied | Alpha::eInherit,
    };
    for (const auto flags : supported) {
        const auto chosen = ChooseCompositeAlpha(flags);
        assert(chosen.has_value());
        assert(static_cast<bool>(flags & *chosen));
    }
    assert(!ChooseCompositeAlpha(Flags{}).has_value());
    std::puts("swapchain composite-alpha policy: OK");
}
