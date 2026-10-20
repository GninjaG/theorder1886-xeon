// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <unordered_set>
#include <vector>
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static bool IsLoadShared(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::LoadSharedU16 ||
           inst.GetOpcode() == IR::Opcode::LoadSharedU32 ||
           inst.GetOpcode() == IR::Opcode::LoadSharedU64;
}

static bool IsWriteShared(const IR::Inst& inst) {
    const IR::Opcode opcode = inst.GetOpcode();
    if (opcode >= IR::Opcode::SharedAtomicIAdd32 && opcode <= IR::Opcode::SharedAtomicCmpSwap64) {
        return !inst.Flags<bool>();
    }
    return opcode == IR::Opcode::WriteSharedU16 || opcode == IR::Opcode::WriteSharedU32 ||
           opcode == IR::Opcode::WriteSharedU64;
}

// Inserts barriers when a shared memory write and read occur in the same basic block.
static void EmitBarrierInBlock(IR::Block* block, bool subgroup = false) {
    const auto emit_barrier = [subgroup](IR::IREmitter& ir) {
        if (subgroup) {
            ir.SubgroupBarrier();
        } else {
            ir.Barrier();
        }
    };
    enum class BarrierAction : u32 {
        None,
        BarrierOnWrite,
        BarrierOnRead,
    };
    BarrierAction action{};
    for (IR::Inst& inst : block->Instructions()) {
        if (IsLoadShared(inst)) {
            if (action == BarrierAction::BarrierOnRead) {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                emit_barrier(ir);
            }
            action = BarrierAction::BarrierOnWrite;
            continue;
        }
        if (IsWriteShared(inst)) {
            if (action == BarrierAction::BarrierOnWrite) {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                emit_barrier(ir);
            }
            action = BarrierAction::BarrierOnRead;
        }
    }
    if (action != BarrierAction::None) {
        IR::IREmitter ir{*block, --block->end()};
        emit_barrier(ir);
    }
}

using NodeSet = std::unordered_set<const IR::Block*>;

static void EmitBarrierAtBlockStart(IR::Block* block) {
    auto insert_point = std::ranges::find_if_not(block->Instructions(), IR::IsPhi);
    IR::IREmitter ir{*block, insert_point};
    ir.Barrier();
}

static bool IsDivergent(const IR::U1& cond) {
    return IR::BreadthFirstSearch(cond, [](IR::Inst* inst) -> std::optional<bool> {
               if (inst->GetOpcode() == IR::Opcode::GetAttributeU32 &&
                   inst->Arg(0).Attribute() == IR::Attribute::LocalInvocationId) {
                   return true;
               }
               return std::nullopt;
           }) == true;
}

// With several GCN waves per group a condition can be uniform inside each wave (ballots,
// readfirstlane, loaded data) and still differ between waves: a group barrier inside it hangs
// the GPU. Only values built from constants, user data and the workgroup id count as uniform.
static bool IsGroupNonUniform(const IR::U1& cond) {
    using Op = IR::Opcode;
    return IR::BreadthFirstSearch(cond, [](IR::Inst* inst) -> std::optional<bool> {
               switch (inst->GetOpcode()) {
               case Op::GetAttribute:
               case Op::GetAttributeU1:
               case Op::GetAttributeU32:
                   if (inst->NumArgs() > 0 && inst->Arg(0).IsImmediate() &&
                       inst->Arg(0).Type() == IR::Type::Attribute &&
                       inst->Arg(0).Attribute() == IR::Attribute::WorkgroupId) {
                       return std::nullopt;
                   }
                   return true;
               case Op::GetExec:
               case Op::GetVcc:
               case Op::GetVccLo:
               case Op::GetVccHi:
               case Op::GetScc:
               case Op::GetScalarRegister:
               case Op::GetVectorRegister:
               case Op::GetVirtualRegister:
               case Op::MaskedBitCount32:
               case Op::LaneId:
               case Op::QuadBroadcast:
               case Op::ReadFirstLane:
               case Op::ReadLane:
               case Op::WriteLane:
               case Op::Ballot:
               case Op::BallotFindLsb:
               case Op::InverseBallot:
               case Op::GroupAny:
                   return true;
               default:
                   break;
               }
               const auto name = IR::NameOf(inst->GetOpcode());
               if (name.starts_with("LoadShared") || name.starts_with("SharedAtomic") ||
                   name.starts_with("LoadBuffer") || name.starts_with("BufferAtomic") ||
                   name.starts_with("Image") || name.starts_with("Load")) {
                   return true;
               }
               return std::nullopt;
           }) == true;
}

static NodeSet FindBarrierRegions(const IR::AbstractSyntaxList& syntax_list) {
    using Type = IR::AbstractSyntaxNode::Type;
    NodeSet regions;
    std::vector<const IR::Block*> open;
    for (const IR::AbstractSyntaxNode& node : syntax_list) {
        switch (node.type) {
        case Type::If:
            open.push_back(node.data.if_node.merge);
            break;
        case Type::Loop:
            open.push_back(node.data.loop.merge);
            break;
        case Type::EndIf:
        case Type::Repeat: {
            const IR::Block* merge =
                node.type == Type::EndIf ? node.data.end_if.merge : node.data.repeat.merge;
            const auto it = std::ranges::find(open, merge);
            if (it != open.end()) {
                open.erase(it, open.end());
            }
            break;
        }
        case Type::Block:
            for (const IR::Inst& inst : node.data.block->Instructions()) {
                if (inst.GetOpcode() == IR::Opcode::Barrier) {
                    regions.insert(open.begin(), open.end());
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
    return regions;
}

// Inserts a barrier after divergent conditional blocks to avoid undefined
// behavior when some threads write and others read from shared memory.
static void EmitBarrierInMergeBlock(const IR::AbstractSyntaxNode::Data& data,
                                    NodeSet& divergence_end, u32& divergence_depth,
                                    bool group_check = false,
                                    const NodeSet* barrier_regions = nullptr) {
    const IR::U1 cond = data.if_node.cond;
    if (barrier_regions && barrier_regions->contains(data.if_node.merge)) {
        return;
    }
    if (group_check ? IsGroupNonUniform(cond) : IsDivergent(cond)) {
        if (divergence_depth == 0) {
            EmitBarrierAtBlockStart(data.if_node.merge);
        }
        ++divergence_depth;
        divergence_end.emplace(data.if_node.merge);
    }
}

// A barrier inside a loop is invalid when different invocations leave on different iterations.
// Mark such loops so their shared-memory synchronization can be deferred to the merge block.
static NodeSet FindDivergentLoops(const IR::AbstractSyntaxList& syntax_list, bool group_check = false,
                                  const NodeSet* barrier_regions = nullptr) {
    NodeSet divergent_loops;
    const auto divergent = [group_check](const IR::U1& cond) {
        return group_check ? IsGroupNonUniform(cond) : IsDivergent(cond);
    };
    const auto game_barrier = [barrier_regions](const IR::Block* merge) {
        return barrier_regions && barrier_regions->contains(merge);
    };
    for (const IR::AbstractSyntaxNode& node : syntax_list) {
        switch (node.type) {
        case IR::AbstractSyntaxNode::Type::Repeat:
            if (divergent(node.data.repeat.cond) && !game_barrier(node.data.repeat.merge)) {
                divergent_loops.emplace(node.data.repeat.merge);
            }
            break;
        case IR::AbstractSyntaxNode::Type::Break:
            if (divergent(node.data.break_node.cond) && !game_barrier(node.data.break_node.merge)) {
                divergent_loops.emplace(node.data.break_node.merge);
            }
            break;
        default:
            break;
        }
    }
    return divergent_loops;
}

static constexpr u32 GcnSubgroupSize = 64;

// The multiwave LDS group-barrier path from v0.7.5 is preserved as the default. The RCA
// (FEAT-005 Class E) kept THEORDER_LDS_BARRIERS_MULTIWAVE as an explicit diagnostic OVERRIDE, not
// as the on/off default: with the env unset the v0.7.5 eligibility heuristic decides (group
// barriers on a wave-split host for a workgroup that spans whole subgroups); env=0 forces the
// group path OFF (subgroup barriers only) and env=1 forces it ON wherever there is LDS to sync on
// a wave-split host. The group-barrier correctness on AMD wave32 is not visually validated here
// (no AMD hardware available); the override exists so that A/B test can run without regressing the
// shipped v0.7.5 default.
LdsBarrierOverride MultiwaveLdsBarrierOverride() {
    static const LdsBarrierOverride selected = [] {
        const char* env = std::getenv("THEORDER_LDS_BARRIERS_MULTIWAVE");
        if (env && env[0] == '0') {
            return LdsBarrierOverride::ForceOff;
        }
        if (env && env[0] == '1') {
            return LdsBarrierOverride::ForceOn;
        }
        return LdsBarrierOverride::Default;
    }();
    return selected;
}

LdsBarrierMode SelectLdsBarrierMode(const LdsKernelShape& shape, LdsBarrierOverride override) {
    if (shape.shared_memory_size == 0) {
        return LdsBarrierMode::None;
    }
    // A host that runs GCN waves in lockstep (needs_lds_barriers == false) never needs explicit
    // LDS barriers and always takes the subgroup path.
    if (!shape.needs_lds_barriers) {
        return LdsBarrierMode::Subgroup;
    }
    // ForceOff (THEORDER_LDS_BARRIERS_MULTIWAVE=0) collapses every wave-split shape onto subgroup
    // barriers, the conservative diagnostic baseline for A/B testing on AMD wave32.
    if (override == LdsBarrierOverride::ForceOff) {
        return LdsBarrierMode::Subgroup;
    }
    // v0.7.5 default heuristic (preserved): the group-barrier path with divergence handling runs
    // for a workgroup that is exactly one HW subgroup, OR - on a wave-split host - for a genuinely
    // multiwave group whose size is a whole number of subgroups.
    const bool wave_split = shape.subgroup_size < GcnSubgroupSize;
    const bool eligible =
        shape.threadgroup_size == GcnSubgroupSize ||
        (wave_split && shape.threadgroup_size > GcnSubgroupSize &&
         shape.threadgroup_size % GcnSubgroupSize == 0);
    return eligible ? LdsBarrierMode::Multiwave : LdsBarrierMode::Subgroup;

void SharedMemoryBarrierPass(IR::Program& program, const RuntimeInfo& runtime_info,
                             const Profile& profile) {
    if (program.info.hw_stage != HwStage::Compute) {
        return;
    }
    const auto& cs_info = runtime_info.hw.cs;
    const u32 shared_memory_size = cs_info.shared_memory_size;
    const u32 threadgroup_size =
        cs_info.workgroup_size[0] * cs_info.workgroup_size[1] * cs_info.workgroup_size[2];
    const LdsBarrierMode mode = SelectLdsBarrierMode(
        {threadgroup_size, shared_memory_size, profile.subgroup_size, profile.needs_lds_barriers},
        MultiwaveLdsBarrierOverride());
    if (mode == LdsBarrierMode::None) {
        return;
    }
    // Subgroup path: GCN waves share LDS in lockstep, but host compilers may reorder the accesses,
    // so keep their order with subgroup memory barriers. The Multiwave path below falls through to
    // the divergence-aware group-barrier emission (v0.7.5 default preserved).
    if (mode == LdsBarrierMode::Subgroup) {
        for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
            if (node.type == IR::AbstractSyntaxNode::Type::Block) {
                EmitBarrierInBlock(node.data.block, true);
            }
        }
        return;
    }
    using Type = IR::AbstractSyntaxNode::Type;
    u32 divergence_depth{};
    NodeSet divergence_end;
    const bool group_check = threadgroup_size != GcnSubgroupSize;
    if (group_check) {
        for (size_t i = 0; i + 1 < program.syntax_list.size(); ++i) {
            if (program.syntax_list[i].type == Type::Return) {
                return;
            }
        }
    }
    const NodeSet barrier_regions = group_check ? FindBarrierRegions(program.syntax_list) : NodeSet{};
    const NodeSet divergent_loops =
        FindDivergentLoops(program.syntax_list, group_check, group_check ? &barrier_regions : nullptr);
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        if (node.type == Type::EndIf) {
            if (divergence_end.contains(node.data.end_if.merge)) {
                --divergence_depth;
            }
            continue;
        }
        if (node.type == Type::If) {
            EmitBarrierInMergeBlock(node.data, divergence_end, divergence_depth, group_check,
                                   group_check ? &barrier_regions : nullptr);
            continue;
        }
        if (node.type == Type::Loop && divergent_loops.contains(node.data.loop.merge)) {
            ++divergence_depth;
            continue;
        }
        if (node.type == Type::Repeat && divergent_loops.contains(node.data.repeat.merge)) {
            ASSERT(divergence_depth > 0);
            --divergence_depth;
            if (divergence_depth == 0) {
                EmitBarrierAtBlockStart(node.data.repeat.merge);
            }
            continue;
        }
        if (node.type == Type::Block && divergence_depth == 0) {
            EmitBarrierInBlock(node.data.block);
        }
    }
}

} // namespace Shader::Optimization
