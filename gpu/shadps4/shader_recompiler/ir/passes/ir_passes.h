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

/// FEAT-005 Class E (HYPOTHESIS, needs AMD wave32 verification): the shape of a compute kernel
/// as it reaches the shared-memory barrier pass, enough to decide which barrier path the pass
/// takes. Kept as a small, deterministic descriptor so barrier-placement can be unit-tested
/// (multiwave-barriers-test) without constructing a full IR::Program.
struct LdsKernelShape {
    u32 threadgroup_size{};   ///< workgroup_size[0]*[1]*[2]
    u32 shared_memory_size{}; ///< bytes of LDS the kernel declares
    bool needs_lds_barriers{};///< Profile::needs_lds_barriers (wave-split host)
};

/// Which barrier strategy SharedMemoryBarrierPass applies for a given kernel shape. See the pass
/// body: no LDS -> none; a single-subgroup group or a host that does not need LDS barriers ->
/// subgroup memory barriers; a multiwave group on a wave-split host -> full group barriers.
enum class LdsBarrierMode : u32 {
    None,      ///< shared_memory_size == 0: nothing to synchronize
    Subgroup,  ///< lockstep single subgroup / host without needs_lds_barriers
    Multiwave, ///< threadgroup spans multiple subgroups on a wave-split host (HYPOTHESIS path)
};

/// true when THEORDER_LDS_BARRIERS_MULTIWAVE is set to 1, opting into the HYPOTHESIS multiwave
/// group-barrier path for A/B testing. Default (unset/0) keeps the shipped behavior, so there is
/// no behavior change unless a tester explicitly opts in.
[[nodiscard]] bool MultiwaveLdsBarriersRequested();

/// The barrier mode the pass selects for `shape`. When `multiwave_requested` is false the
/// multiwave group-barrier path is NOT taken even on a wave-split host (it falls back to the
/// subgroup path), which is how the pass stays gated behind the env flag.
[[nodiscard]] LdsBarrierMode SelectLdsBarrierMode(const LdsKernelShape& shape,
                                                  bool multiwave_requested);
void SharedMemoryToStoragePass(IR::Program& program, const RuntimeInfo& runtime_info,
                               const Profile& profile);

} // namespace Shader::Optimization
