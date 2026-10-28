// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <initializer_list>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace Vulkan {

// VK_KHR_swapchain requires compositeAlpha to be one of supportedCompositeAlpha.
// Opaque is preferred for the game, but neither Inherit nor any other mode
// can be assumed available on every surface (including after monitor changes).
[[nodiscard]] constexpr std::optional<vk::CompositeAlphaFlagBitsKHR> ChooseCompositeAlpha(
    vk::CompositeAlphaFlagsKHR supported) noexcept {
    for (const auto candidate : {vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                 vk::CompositeAlphaFlagBitsKHR::eInherit,
                                 vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
                                 vk::CompositeAlphaFlagBitsKHR::ePostMultiplied}) {
        if (supported & candidate) {
            return candidate;
        }
    }
    return std::nullopt;
}

} // namespace Vulkan
