// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the real InsertSsboStoreLoadBarrier pass on synthetic, already-patched
// resource bindings. No GPU, game data or Vulkan driver is required.
#include <cassert>
#include <cstdio>
#include <vector>

#include "common/object_pool.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/profile.h"

using Shader::IR::Opcode;
using Shader::IR::Value;

static std::vector<Opcode> Apply(Opcode write, Opcode read, bool same_binding,
                                 bool enabled, bool store_first = true) {
    Shader::Info info{};
    Shader::IR::Program program{info};
    Common::ObjectPool<Shader::IR::Inst> pool;
    Shader::IR::Block block(pool);
    program.blocks.push_back(&block);
    // ResourcePatchingPass has already replaced Arg(0) with a U32 index.
    const Value write_binding{std::uint32_t{3}};
    const Value read_binding{std::uint32_t{same_binding ? 3u : 4u}};
    const Value addr{std::uint32_t{0}};
    const Value payload{1.0f};
    const auto append_store = [&] {
        block.AppendNewInst(write, {write_binding, addr, payload});
    };
    const auto append_load = [&] {
        block.AppendNewInst(read, {read_binding, addr});
    };
    if (store_first) {
        append_store();
        append_load();
    } else {
        append_load();
        append_store();
    }
    Shader::Profile profile{};
    profile.needs_ssbo_store_load_barrier = enabled;
    Shader::Optimization::InsertSsboStoreLoadBarrier(program, profile);
    std::vector<Opcode> actual;
    for (const auto& inst : block.Instructions()) {
        actual.push_back(inst.GetOpcode());
    }
    return actual;
}

int main() {
    const auto formatted_write = Opcode::StoreBufferFormatF32;
    const auto formatted_read = Opcode::LoadBufferFormatF32;
    const auto raw_write = Opcode::StoreBufferF32;
    const auto raw_read = Opcode::LoadBufferF32;
    const auto barrier = Opcode::DeviceMemoryBarrier;
    // Previously both formatted opcodes were invisible to the pass.
    assert((Apply(formatted_write, formatted_read, true, true) ==
            std::vector<Opcode>{formatted_write, barrier, formatted_read}));
    assert((Apply(formatted_write, raw_read, true, true) ==
            std::vector<Opcode>{formatted_write, barrier, raw_read}));
    assert((Apply(raw_write, formatted_read, true, true) ==
            std::vector<Opcode>{raw_write, barrier, formatted_read}));
    assert((Apply(raw_write, raw_read, true, true) ==
            std::vector<Opcode>{raw_write, barrier, raw_read}));
    // Distinct bindings are unrelated and never gain the barrier.
    assert((Apply(formatted_write, formatted_read, false, true) ==
            std::vector<Opcode>{formatted_write, formatted_read}));
    // All new behavior is gated on the pre-existing host capability flag.
    assert((Apply(formatted_write, formatted_read, true, false) ==
            std::vector<Opcode>{formatted_write, formatted_read}));
    // Preserve the original pass's conservative global store/load analysis;
    // do not claim instruction-level dependence for unordered control flow.
    assert((Apply(formatted_write, formatted_read, true, true, false) ==
            std::vector<Opcode>{formatted_read, formatted_write, barrier}));
    std::puts("formatted-SSBO barriers: IR pass regression tests passed");
}
