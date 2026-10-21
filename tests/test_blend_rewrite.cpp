// SPDX-License-Identifier: GPL-2.0-or-later
// Color-blend classification and GNM MIN/MAX rewrite. No game, Vulkan device or window required.
#include <array>
#include <cassert>
#include <cstdio>

#include "video_core/renderer_vulkan/vk_blend_rewrite.h"

using namespace Vulkan;
using F = AmdGpu::BlendControl::BlendFactor;
using Fn = AmdGpu::BlendControl::BlendFunc;
using Nf = AmdGpu::NumberFormat;

static BlendRewriteResult Rewrite(BlendEquation& eq, Nf format = Nf::Unorm,
                                  BlendChannel channel = BlendChannel::Color) {
    return RewriteScaledMinMaxBlend(eq, format, channel);
}

// The same matrix can exercise the rasterizer expression in a standalone call-site proof.
template <typename Classify>
static void TestAdditiveTargetClassification(Classify classify) {
    std::array<AmdGpu::BlendControl, 8> blends{};
    bool ok = true;
    const auto check = [&](const char* name, unsigned cb, bool bypass, bool expected) {
        if (classify(blends, cb, bypass) != expected) {
            std::fprintf(stderr, "additive classification failed: %s\n", name);
            ok = false;
        }
    };
    const auto additive = [](F source, Fn function = Fn::Add) {
        AmdGpu::BlendControl blend{};
        blend.enable = 1;
        blend.color_src_factor = source;
        blend.color_dst_factor = F::One;
        blend.color_func = function;
        return blend;
    };
    for (const auto source : {F::One, F::SrcAlpha}) {
        blends[0] = additive(source);
        check("ADD accepts supported source", 0, false, true);
        check("blend bypass rejects ADD", 0, true, false);
        blends[0].enable = 0;
        check("disabled blending rejects ADD", 0, false, false);
        blends[0] = additive(source);
        blends[0].color_dst_factor = F::Zero;
        check("destination must be One", 0, false, false);
        for (const auto function : {Fn::Subtract, Fn::ReverseSubtract, Fn::Min, Fn::Max}) {
            blends[0] = additive(source, function);
            check("non-ADD rejects supported source", 0, false, false);
        }
    }
    blends[0] = additive(F::SrcColor);
    check("unsupported source rejects ADD", 0, false, false);

    blends[0] = additive(F::One);
    blends[1] = {};
    check("MRT0 additive", 0, false, true);
    check("opaque MRT1 independent of additive MRT0", 1, false, false);
    std::swap(blends[0], blends[1]);
    check("MRT1 additive independent of opaque MRT0", 1, false, true);
    check("current MRT1 bypass", 1, true, false);
    assert(ok);
}

int main() {
    TestAdditiveTargetClassification([](const auto& blends, unsigned cb, bool bypass) {
        return IsAdditiveColorTarget(blends[cb], bypass);
    });
    // Not MIN/MAX, or MIN/MAX with unit factors: untouched.
    {
        BlendEquation eq{F::SrcAlpha, Fn::Add, F::OneMinusSrcAlpha};
        assert(Rewrite(eq) == BlendRewriteResult::Native);
        assert(eq.src_factor == F::SrcAlpha && eq.function == Fn::Add);
        BlendEquation mm{F::One, Fn::Min, F::One};
        assert(Rewrite(mm) == BlendRewriteResult::Native && mm.function == Fn::Min);
    }
    // MIN/MAX(s*s, d*d): native operation, squared by a second draw (God of War III shadows).
    {
        BlendEquation eq{F::SrcColor, Fn::Min, F::DstColor};
        assert(Rewrite(eq) == BlendRewriteResult::Squared && eq.function == Fn::Min);
        BlendEquation alpha{F::SrcAlpha, Fn::Max, F::DstAlpha};
        assert(Rewrite(alpha, Nf::Srgb, BlendChannel::Alpha) == BlendRewriteResult::Squared);
        // The color channel only squares through color factors.
        BlendEquation color{F::SrcAlpha, Fn::Max, F::DstAlpha};
        assert(Rewrite(color) == BlendRewriteResult::Unsupported);
    }
    // A zero factor: exact ADD. MIN(x, 0) = 0 and MAX(x, 0) = x on unsigned normalized targets.
    {
        BlendEquation min{F::One, Fn::Min, F::Zero};
        assert(Rewrite(min) == BlendRewriteResult::Exact);
        assert(min.function == Fn::Add && min.src_factor == F::Zero && min.dst_factor == F::Zero);
        BlendEquation max{F::Zero, Fn::Max, F::One};
        assert(Rewrite(max) == BlendRewriteResult::Exact);
        assert(max.function == Fn::Add && max.src_factor == F::Zero && max.dst_factor == F::One);
    }
    // Signed or float targets cannot use those identities.
    {
        BlendEquation eq{F::SrcColor, Fn::Min, F::DstColor};
        assert(Rewrite(eq, Nf::Float) == BlendRewriteResult::Unsupported);
        BlendEquation other{F::SrcAlpha, Fn::Min, F::OneMinusSrcAlpha};
        assert(Rewrite(other) == BlendRewriteResult::Unsupported);
    }
    std::puts("blend-rewrite-test: OK");
    return 0;
}
