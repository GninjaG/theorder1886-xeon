// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/program.h"

namespace Shader {
struct Profile;
void InjectClipDistanceAttributes(IR::Program& program, RuntimeInfo& runtime_info);
} // namespace Shader

namespace Shader::Optimization {

void SsaRewritePass(IR::Program& program);
void SsaRepairPass(IR::Program& program);
void PhiSimplificationPass(IR::Program& program);
void InverseBallotEliminationPass(IR::Program& program);
void LowerWave64BallotPass(IR::Program& program, const RuntimeInfo& runtime_info,
                           const Profile& profile);
void LowerHardwareIntrinsics(IR::Program& program);
void LowerPhisToRegsPass(IR::Program& program);
void DeadCodeEliminationPass(IR::Program& program);
/// port: exact integers from interpolated float varyings on GPUs without manual interpolation.
void InterpolatedIntegerPass(IR::Program& program);
void ConstantPropagationPass(IR::BlockList& program);
void FlattenExtendedUserdataPass(IR::Program& program);
void ReadLaneEliminationPass(IR::Program& program);
ResourceDiscoveryList ResourceDiscoverPass(IR::Program& program, const Profile& profile);
void ResourcePatchingPass(Shader::Info& info, const ResourceDiscoveryList& sharp_usages,
                          const Profile& profile);
void InsertSsboStoreLoadBarrier(IR::Program& program, const Profile& profile);
void CollectShaderInfoPass(IR::Program& program, const Profile& profile);
void LowerBufferFormatToRaw(IR::Program& program);
void LowerUserClipPlanes(IR::Program& program, const RuntimeInfo& runtime_info);
void LowerFp64ToFp32(IR::Program& program);
void RingAccessElimination(const IR::Program& program, const RuntimeInfo& runtime_info);
void TessellationPreprocess(IR::Program& program, RuntimeInfo& runtime_info);
void HullShaderTransform(IR::Program& program, const RuntimeInfo& runtime_info);
void DomainShaderTransform(const IR::Program& program, const RuntimeInfo& runtime_info);
void SharedMemoryBarrierPass(IR::Program& program, const RuntimeInfo& runtime_info,
                             const Profile& profile);
void SharedMemorySimplifyPass(IR::Program& program, const Profile& profile);

/// The shape of a compute kernel as it reaches the shared-memory barrier pass, enough to decide
/// which barrier path the pass takes. Kept as a small, deterministic descriptor so barrier
/// placement can be unit-tested (multiwave-barriers-test) without constructing a full IR::Program.
struct LdsKernelShape {
    u32 threadgroup_size{};   ///< workgroup_size[0]*[1]*[2]
    u32 shared_memory_size{}; ///< bytes of LDS the kernel declares
    u32 subgroup_size{};      ///< Profile::subgroup_size (host HW subgroup width)
    bool needs_lds_barriers{};///< Profile::needs_lds_barriers (wave-split host)
};

/// Which barrier strategy SharedMemoryBarrierPass applies for a given kernel shape. See the pass
/// body: no LDS -> none; a single-subgroup group or a host that does not need LDS barriers ->
/// subgroup memory barriers; a multiwave group on a wave-split host -> full group barriers.
enum class LdsBarrierMode : u32 {
    None,      ///< shared_memory_size == 0: nothing to synchronize
    Subgroup,  ///< lockstep single subgroup / host without needs_lds_barriers
    Multiwave, ///< threadgroup spans multiple subgroups on a wave-split host (v0.7.5 default)
};

/// THEORDER_LDS_BARRIERS_MULTIWAVE as a diagnostic override of the v0.7.5 multiwave default.
/// Default (env unset) keeps the shipped v0.7.5 eligibility heuristic; ForceOff (=0) and
/// ForceOn (=1) let a tester A/B the multiwave group-barrier path on AMD wave32 hardware, whose
/// visual correctness is not yet validated here.
enum class LdsBarrierOverride : u32 {
    Default,   ///< env unset: v0.7.5 eligibility heuristic decides
    ForceOff,  ///< THEORDER_LDS_BARRIERS_MULTIWAVE=0: subgroup barriers only
    ForceOn,   ///< THEORDER_LDS_BARRIERS_MULTIWAVE=1: group path wherever a wave-split host has LDS
};

/// The override selected from THEORDER_LDS_BARRIERS_MULTIWAVE for this process.
[[nodiscard]] LdsBarrierOverride MultiwaveLdsBarrierOverride();

/// The barrier mode the pass selects for `shape`. With LdsBarrierOverride::Default the v0.7.5
/// heuristic chooses the multiwave group path on a wave-split host for whole-subgroup workgroups;
/// ForceOff collapses to subgroup barriers, ForceOn widens the group path on wave-split hosts.
[[nodiscard]] LdsBarrierMode SelectLdsBarrierMode(const LdsKernelShape& shape,
                                                  LdsBarrierOverride override);
void SharedMemoryToStoragePass(IR::Program& program, const RuntimeInfo& runtime_info,
                               const Profile& profile);

} // namespace Shader::Optimization
