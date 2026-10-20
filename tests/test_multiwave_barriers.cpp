// SPDX-License-Identifier: GPL-2.0-or-later
// FEAT-005 Class E (HYPOTHESIS, needs AMD wave32 verification): barrier-placement decision for the
// shared-memory barrier pass, asserted for known compute-kernel shapes without constructing a full
// IR::Program (modelled on tests/test_wave_stats.cpp / tests/test_pass_census.cpp, linking
// theordergpu like motion-shader-test). The multiwave group-barrier path stays behind the
// THEORDER_LDS_BARRIERS_MULTIWAVE env guard; this test pins the shipped decision so the gate
// cannot silently flip the default behavior.
#include <cassert>
#include <cstdio>

#include "shader_recompiler/ir/passes/ir_passes.h"

using Shader::Optimization::LdsBarrierMode;
using Shader::Optimization::LdsKernelShape;
using Shader::Optimization::SelectLdsBarrierMode;

int main() {
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
#endif
    // No shared memory declared: nothing to synchronize regardless of shape or host.
    assert(SelectLdsBarrierMode({64, 0, true}, true) == LdsBarrierMode::None);
    assert(SelectLdsBarrierMode({256, 0, true}, true) == LdsBarrierMode::None);

    // A host that does not need LDS barriers (GCN-lockstep) always takes the subgroup path.
    assert(SelectLdsBarrierMode({64, 1024, false}, true) == LdsBarrierMode::Subgroup);
    assert(SelectLdsBarrierMode({256, 1024, false}, true) == LdsBarrierMode::Subgroup);

    // Shipped group-barrier path: workgroup == one HW subgroup (64) on a host needing barriers.
    // This is the only shape that reaches the divergence-handling group-barrier emission, and it
    // does so whether or not the multiwave flag is opted into (default behavior is unchanged).
    assert(SelectLdsBarrierMode({64, 1024, true}, false) == LdsBarrierMode::Multiwave);
    assert(SelectLdsBarrierMode({64, 1024, true}, true) == LdsBarrierMode::Multiwave);

    // A genuinely multiwave group (threadgroup spans multiple subgroups) on a wave-split host
    // takes subgroup barriers today - the HYPOTHESIS wave32 change must NOT turn this into the
    // group path by default; the env flag is a test hook and does not alter this shape's mode.
    assert(SelectLdsBarrierMode({128, 1024, true}, false) == LdsBarrierMode::Subgroup);
    assert(SelectLdsBarrierMode({128, 1024, true}, true) == LdsBarrierMode::Subgroup);
    assert(SelectLdsBarrierMode({256, 2048, true}, true) == LdsBarrierMode::Subgroup);

    std::puts("multiwave-barriers-test: OK");
    return 0;
}
