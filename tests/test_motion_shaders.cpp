// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual SPIR-V backend without opening a window or loading the game.
// Output is validated by spirv-val (see docs/upscaler.md).
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <setjmp.h>
#include <utility>
#include <vector>
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/resource.h"

static bool HasMotionAddressSpecId(const std::vector<u32>& code) {
    constexpr u32 OpDecorate = 71;
    constexpr u32 DecorationSpecId = 1;
    constexpr u32 ParamsSpecId = 0x6d76;
    constexpr u32 PositionsSpecId = 0x6d77;
    for (size_t i = 5; i < code.size();) {
        const u32 instruction = code[i];
        const u32 word_count = instruction >> 16;
        if (word_count == 0 || i + word_count > code.size()) {
            return true;
        }
        if ((instruction & 0xffffu) == OpDecorate && word_count >= 4 &&
            code[i + 2] == DecorationSpecId &&
            (code[i + 3] == ParamsSpecId || code[i + 3] == PositionsSpecId)) {
            return true;
        }
        i += word_count;
    }
    return false;
}

static bool HasMotionAddressPushOffsets(const std::vector<u32>& code) {
    constexpr u32 OpMemberDecorate = 72;
    constexpr u32 OpVariable = 59;
    constexpr u32 OpConstant = 43;
    constexpr u32 OpAccessChain = 65;
    constexpr u32 DecorationOffset = 35;
    constexpr u32 StorageClassPushConstant = 9;
    std::array<bool, 2> found{};
    std::array<bool, 2> accessed{};
    const std::array<u32, 2> indices{Shader::PushData::MotionParamsAddressIndex,
                                     Shader::PushData::MotionPositionsAddressIndex};
    const std::array<u32, 2> offsets{
        static_cast<u32>(offsetof(Shader::PushData, motion_params_address)),
        static_cast<u32>(offsetof(Shader::PushData, motion_positions_address))};
    std::vector<u32> push_blocks;
    std::vector<std::pair<u32, u32>> constants;
    for (size_t i = 5; i < code.size();) {
        const u32 instruction = code[i];
        const u32 word_count = instruction >> 16;
        if (word_count == 0 || i + word_count > code.size()) {
            return false;
        }
        const u32 opcode = instruction & 0xffffu;
        if (opcode == OpVariable && word_count >= 4 &&
            code[i + 3] == StorageClassPushConstant) {
            push_blocks.push_back(code[i + 2]);
        } else if (opcode == OpConstant && word_count >= 4) {
            constants.emplace_back(code[i + 2], code[i + 3]);
        }
        i += word_count;
    }
    for (size_t i = 5; i < code.size();) {
        const u32 instruction = code[i];
        const u32 word_count = instruction >> 16;
        if (word_count == 0 || i + word_count > code.size()) {
            return false;
        }
        const u32 opcode = instruction & 0xffffu;
        if (opcode == OpMemberDecorate && word_count >= 5 &&
            code[i + 3] == DecorationOffset) {
            for (size_t member = 0; member < indices.size(); ++member) {
                found[member] |= code[i + 2] == indices[member] && code[i + 4] == offsets[member];
            }
        } else if (opcode == OpAccessChain && word_count >= 5 &&
                   std::ranges::find(push_blocks, code[i + 3]) != push_blocks.end()) {
            for (const auto& [id, value] : constants) {
                for (size_t member = 0; member < indices.size(); ++member) {
                    accessed[member] |= code[i + 4] == id && value == indices[member];
                }
            }
        }
        i += word_count;
    }
    return found[0] && found[1] && accessed[0] && accessed[1];
}

static bool HasMotionAddressPushMembers(const std::vector<u32>& code) {
    constexpr u32 OpMemberDecorate = 72;
    constexpr u32 DecorationOffset = 35;
    const std::array<u32, 2> indices{Shader::PushData::MotionParamsAddressIndex,
                                     Shader::PushData::MotionPositionsAddressIndex};
    const std::array<u32, 2> offsets{
        static_cast<u32>(offsetof(Shader::PushData, motion_params_address)),
        static_cast<u32>(offsetof(Shader::PushData, motion_positions_address))};
    for (size_t i = 5; i < code.size();) {
        const u32 instruction = code[i];
        const u32 word_count = instruction >> 16;
        if (word_count == 0 || i + word_count > code.size()) {
            return true;
        }
        if ((instruction & 0xffffu) == OpMemberDecorate && word_count >= 5 &&
            code[i + 3] == DecorationOffset) {
            for (size_t member = 0; member < indices.size(); ++member) {
                if (code[i + 2] == indices[member] && code[i + 4] == offsets[member]) {
                    return true;
                }
            }
        }
        i += word_count;
    }
    return false;
}


int main(int argc, char** argv) {
    using namespace Shader;
    const std::filesystem::path dir = argc > 1 ? argv[1] : ".";
    std::filesystem::create_directories(dir);
    MotionVectors::params_address = 0x10000;
    MotionVectors::positions_address = 0x20000;
    for (bool vertex : {true, false}) {
        for (bool motion : {false, true}) {
            Info info{};
            info.hw_stage = vertex ? HwStage::Vertex : HwStage::Fragment;
            info.sw_stage = vertex ? SwStage::Vertex : SwStage::Fragment;
            RuntimeInfo runtime{};
            runtime.Initialize(info.hw_stage, info.sw_stage);
            if (vertex) {
                runtime.hw.vs.motion_vectors = motion;
            } else {
                runtime.hw.fs.motion_vectors = motion;
                runtime.hw.fs.color_buffers[0].num_format = AmdGpu::NumberFormat::Float;
            }
            Common::ObjectPool<IR::Inst> pool;
            IR::Block block(pool);
            IR::IREmitter ir(block);
            const auto output = vertex ? IR::Attribute::Position0 : IR::Attribute::RenderTarget0;
            for (u32 component = 0; component < 4; ++component) {
                info.stores.Set(output, component);
                if (!vertex && component < 2) {
                    info.loads.Set(IR::Attribute::FragCoord, component);
                    ir.SetAttribute(output, ir.GetAttribute(IR::Attribute::FragCoord, component), component);
                } else {
                    ir.SetAttribute(output, ir.Imm32(component == 3 ? 1.0f : 0.0f), component);
                }
            }
            ir.Epilogue();
            IR::Program program(info);
            program.blocks.push_back(&block);
            program.syntax_list.push_back({.data = {.block = &block},
                                           .type = IR::AbstractSyntaxNode::Type::Block});
            program.syntax_list.push_back({.type = IR::AbstractSyntaxNode::Type::Return});
            Profile profile{};
            profile.supported_spirv = 0x00010600;
            profile.support_int64 = true;
            Backend::Bindings bindings{};
            const auto code = Backend::SPIRV::EmitSPIRV(profile, runtime, program, bindings);
            if (vertex && motion &&
                (HasMotionAddressSpecId(code) || !HasMotionAddressPushOffsets(code))) {
                std::cerr << "motion vertex shader must load session addresses from push constants\n";
                return 2;
            }
            if (vertex && !motion && HasMotionAddressPushMembers(code)) {
                std::cerr << "non-motion vertex shader must not declare motion address members\n";
                return 3;
            }
            const auto path = dir / (std::string(vertex ? "vertex" : "fragment") +
                                      (motion ? "-motion.spv" : "-plain.spv"));
            std::ofstream out(path, std::ios::binary);
            out.write(reinterpret_cast<const char*>(code.data()), code.size() * sizeof(u32));
            if (!out) { return 1; }
        }
    }
}
