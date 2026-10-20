// SPDX-License-Identifier: GPL-2.0-or-later
// FEAT-005 Class D (HYPOTHESIS, deterministic): the host-fallback policy for a guest
// 2_10_10_10 Snorm surface must stay on a SIGNED decode path. On a host that advertises
// eA2B10G10R10SnormPack32 (e.g. NVIDIA RTX 5060) the native Snorm view is kept; on a host that
// lacks it (the #39 AMD RX 9070 report) the fallback must be eA2B10G10R10UintPack32 read as raw
// integers with an in-shader *2-1 signed expand - NEVER a bare eA2B10G10R10UnormPack32
// reinterpret, which drops the sign bit and shifts the hue toward green/blue in the Chapter-5
// flares. No game, Vulkan device or window required (modelled on tests/test_aliasing.cpp).
//
// Red/green demonstration: a bare-Unorm policy (BareUnormHostView below) is asserted to FAIL the
// sign-preservation check, while the shipped policy (LiverpoolToVK::Snorm2101010HostView) passes.
#include <cassert>
#include <cstdio>

#include "video_core/renderer_vulkan/liverpool_to_vk.h"

using namespace Vulkan;

// A deliberately-wrong "bare Unorm reinterpret" policy, kept here only so the test can show the
// red state the shipped policy must never match (this is the v0.7.5 regression being fixed).
static constexpr vk::Format BareUnormHostView(bool host_supports_snorm_pack32) {
    return host_supports_snorm_pack32 ? vk::Format::eA2B10G10R10SnormPack32
                                      : vk::Format::eA2B10G10R10UnormPack32;
}

int main() {
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
#endif
    using LiverpoolToVK::Snorm2101010FallbackPreservesSign;
    using LiverpoolToVK::Snorm2101010HostView;

    // The sign-preservation predicate: Snorm and Uint keep the signed decode; Unorm does not.
    assert(Snorm2101010FallbackPreservesSign(vk::Format::eA2B10G10R10SnormPack32));
    assert(Snorm2101010FallbackPreservesSign(vk::Format::eA2B10G10R10UintPack32));
    assert(!Snorm2101010FallbackPreservesSign(vk::Format::eA2B10G10R10UnormPack32));

    // A host with the native Snorm pack format keeps it either way (NVIDIA path, unchanged).
    assert(Snorm2101010HostView(true) == vk::Format::eA2B10G10R10SnormPack32);
    assert(BareUnormHostView(true) == vk::Format::eA2B10G10R10SnormPack32);

    // A host WITHOUT the native Snorm pack format: the shipped policy stays on the signed Uint
    // view; the bare-Unorm policy is the regression.
    const vk::Format shipped = Snorm2101010HostView(false);
    const vk::Format bare = BareUnormHostView(false);
    assert(shipped == vk::Format::eA2B10G10R10UintPack32);
    assert(bare == vk::Format::eA2B10G10R10UnormPack32);

    // Green: the shipped fallback preserves sign. Red (demonstrated): the bare-Unorm fallback
    // does NOT - the policy under test must choose the signed path, not this one.
    assert(Snorm2101010FallbackPreservesSign(shipped));
    assert(!Snorm2101010FallbackPreservesSign(bare));

    std::puts("format-remap-test: OK");
    return 0;
}
