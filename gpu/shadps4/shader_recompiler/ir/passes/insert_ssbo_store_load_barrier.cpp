// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iterator>
#include <set>
#include <utility>
#include <vector>

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

namespace {

bool IsStoreBuffer(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::StoreBufferU8:
    case IR::Opcode::StoreBufferU16:
    case IR::Opcode::StoreBufferU32:
    case IR::Opcode::StoreBufferU32x2:
    case IR::Opcode::StoreBufferU32x3:
    case IR::Opcode::StoreBufferU32x4:
    case IR::Opcode::StoreBufferU64:
    case IR::Opcode::StoreBufferF32:
    case IR::Opcode::StoreBufferF32x2:
    case IR::Opcode::StoreBufferF32x3:
    case IR::Opcode::StoreBufferF32x4:
    // This pass runs before LowerBufferFormatToRaw. Formatted stores can
    // alias ordinary SSBO loads and require the same ordering protection.
    case IR::Opcode::StoreBufferFormatF32:
        return true;
    default:
        return false;
    }
}

bool IsLoadBuffer(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU8:
    case IR::Opcode::LoadBufferU16:
    case IR::Opcode::LoadBufferU32:
    case IR::Opcode::LoadBufferU32x2:
    case IR::Opcode::LoadBufferU32x3:
    case IR::Opcode::LoadBufferU32x4:
    case IR::Opcode::LoadBufferU64:
    case IR::Opcode::LoadBufferF32:
    case IR::Opcode::LoadBufferF32x2:
    case IR::Opcode::LoadBufferF32x3:
    case IR::Opcode::LoadBufferF32x4:
    case IR::Opcode::LoadBufferFormatF32:
        return true;
    default:
        return false;
    }
}

} // namespace

void InsertSsboStoreLoadBarrier(IR::Program& program, const Profile& profile) {
    // GCN hardware forwards a same-address buffer store to a following buffer load within the same
    // dispatch, so the original shader relies on that implicit forwarding. NVIDIA drivers do not
    // guarantee it for non-coherent SSBO accesses, making the load observe stale data. Insert a
    // device memory barrier after stores of any buffer that is both stored and loaded.
    // Formatted, raw and atomic buffer operations are still present at this pass.
    if (!profile.needs_ssbo_store_load_barrier) {
        return;
    }

    std::set<u32> stored_buffers;
    std::set<u32> loaded_buffers;
    std::set<u32> plain_loaded_buffers;
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            // Atomics read, modify and write the same resource. Account for
            // both accesses instead of treating them as an unrelated opcode.
            const bool is_atomic = IsBufferAtomic(inst);
            if (IsStoreBuffer(inst) || is_atomic) {
                stored_buffers.insert(inst.Arg(0).U32());
            }
            if (IsLoadBuffer(inst) || is_atomic) {
                loaded_buffers.insert(inst.Arg(0).U32());
            }
            if (IsLoadBuffer(inst)) {
                plain_loaded_buffers.insert(inst.Arg(0).U32());
            }
        }
    }

    std::vector<std::pair<IR::Block*, IR::Block::iterator>> insert_at;
    for (IR::Block* block : program.blocks) {
        for (auto it = block->Instructions().begin(); it != block->Instructions().end(); ++it) {
            IR::Inst& inst = *it;
            if (!IsStoreBuffer(inst) && !IsBufferAtomic(inst)) {
                continue;
            }
            const u32 binding = inst.Arg(0).U32();
            // An atomic-only binding already uses atomic operations. Adding
            // barriers after every standalone atomic would cause extra GPU
            // work without addressing a raw store/load coherence hazard.
            const bool ordered_with_plain_load =
                !IsBufferAtomic(inst) || plain_loaded_buffers.count(binding) != 0;
            if (stored_buffers.count(binding) && loaded_buffers.count(binding) &&
                ordered_with_plain_load) {
                insert_at.emplace_back(block, std::next(it));
            }
        }
    }

    for (auto [block, pos] : insert_at) {
        IR::IREmitter ir{*block, pos};
        ir.DeviceMemoryBarrier();
    }
}

} // namespace Shader::Optimization
