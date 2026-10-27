// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Narrow, opt-in A/B for one change between windows-v0.6.1 and windows-v0.7.0.
// This must not disable the oversized-pitch/64x-padding rejection or other
// invalid descriptor/mapping guards.
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace Vulkan::TextureSizeAb {

inline constexpr std::uint64_t CurrentMaxGuestBytes = 256ULL << 20;
inline constexpr std::uint64_t LegacyMaxGuestBytes = 512ULL << 20;

// No arbitrary sizes: a typo or inherited diagnostic cannot remove the cap.
constexpr std::uint64_t SelectLimit(std::string_view choice) noexcept {
    return choice == "512" ? LegacyMaxGuestBytes : CurrentMaxGuestBytes;
}

constexpr bool Reject(std::uint64_t guest_bytes, bool oversized_padding,
                      std::uint64_t max_guest_bytes) noexcept {
    return oversized_padding || guest_bytes > max_guest_bytes;
}

inline std::uint64_t LimitBytes() noexcept {
    static const std::uint64_t limit = [] {
        const char* value = std::getenv("THEORDER_TEXTURE_MAX_MIB");
        return SelectLimit(value ? std::string_view(value) : std::string_view{});
    }();
    return limit;
}

} // namespace Vulkan::TextureSizeAb
