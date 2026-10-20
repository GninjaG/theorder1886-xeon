// SPDX-License-Identifier: GPL-2.0-or-later
// Barrier-placement decision for the shared-memory barrier pass, asserted for known compute-kernel
// shapes without constructing a full IR::Program (modelled on tests/test_wave_stats.cpp /
// tests/test_pass_census.cpp, linking theordergpu like motion-shader-test).
//
// The v0.7.5 multiwave LDS group-barrier default is PRESERVED: on a wave-split host a genuinely
// multiwave group whose size is a whole number of subgroups takes the group path by default.
// THEORDER_LDS_BARRIERS_MULTIWAVE is a diagnostic OVERRIDE (ForceOff/ForceOn), not the on/off
// default; this test pins both the default heuristic and the override so neither can silently
// regress the shipped v0.7.5 behavior. Group-barrier visual correctness on AMD wave32 is not
// validated here (no AMD hardware); only the mode-selection logic is covered.
#include <cassert>
#include <cstdio>

#include "shader_recompiler/ir/passes/ir_passes.h"

using Shader::Optimization::LdsBarrierMode;
using Shader::Optimization::LdsBarrierOverride;
using Shader::Optimization::LdsKernelShape;
using Shader::Optimization::SelectLdsBarrierMode;

// Shape fields: {threadgroup_size, shared_memory_size, subgroup_size, needs_lds_barriers}.
int main() {
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
#endif
    constexpr auto kDefault = LdsBarrierOverride::Default;
    constexpr auto kForceOff = LdsBarrierOverride::ForceOff;
    constexpr auto kForceOn = LdsBarrierOverride::ForceOn;

    // No shared memory declared: nothing to synchronize regardless of shape, host or override.
    assert(SelectLdsBarrierMode({64, 0, 64, true}, kDefault) == LdsBarrierMode::None);
    assert(SelectLdsBarrierMode({256, 0, 32, true}, kForceOn) == LdsBarrierMode::None);

    // A host that does not need LDS barriers (GCN-lockstep) always takes the subgroup path, even
    // when the override forces the multiwave path on.
    assert(SelectLdsBarrierMode({64, 1024, 64, false}, kDefault) == LdsBarrierMode::Subgroup);
    assert(SelectLdsBarrierMode({256, 1024, 32, false}, kForceOn) == LdsBarrierMode::Subgroup);

    // Workgroup == one HW subgroup (64) on a host needing barriers: group path in every override.
    assert(SelectLdsBarrierMode({64, 1024, 64, true}, kDefault) == LdsBarrierMode::Multiwave);
    assert(SelectLdsBarrierMode({64, 1024, 64, true}, kForceOff) == LdsBarrierMode::Subgroup);
    assert(SelectLdsBarrierMode({64, 1024, 64, true}, kForceOn) == LdsBarrierMode::Multiwave);

    // v0.7.5 DEFAULT: a genuinely multiwave group on a wave-split host (subgroup 32, size a whole
    // number of subgroups) takes the group path by default. ForceOff collapses it to subgroup.
    assert(SelectLdsBarrierMode({128, 1024, 32, true}, kDefault) == LdsBarrierMode::Multiwave);
    assert(SelectLdsBarrierMode({128, 1024, 32, true}, kForceOff) == LdsBarrierMode::Subgroup);
    assert(SelectLdsBarrierMode({128, 1024, 32, true}, kForceOn) == LdsBarrierMode::Multiwave);
    assert(SelectLdsBarrierMode({256, 2048, 32, true}, kDefault) == LdsBarrierMode::Multiwave);

    // A multiwave group on a host that is NOT wave-split (subgroup already 64) is not eligible for
    // the multiwave widening and keeps subgroup barriers; ForceOn does not change a non-split host.
    assert(SelectLdsBarrierMode({128, 1024, 64, true}, kDefault) == LdsBarrierMode::Subgroup);
    assert(SelectLdsBarrierMode({128, 1024, 64, true}, kForceOn) == LdsBarrierMode::Subgroup);

    // A wave-split group size that is not a whole number of subgroups stays on subgroup barriers.
    assert(SelectLdsBarrierMode({96, 1024, 32, true}, kDefault) == LdsBarrierMode::Subgroup);

    std::puts("multiwave-barriers-test: OK");
    return 0;
}
