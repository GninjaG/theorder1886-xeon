// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Shader {

struct Profile {
    u32 max_viewport_width{};
    u32 max_viewport_height{};
    u32 max_shared_memory_size{};
    u32 supported_spirv{0x00010000};
    u32 subgroup_size{};
    u32 sparse_page_shift{};
    bool support_int8{};
    bool support_int16{};
    bool support_int64{};
    bool support_float16{};
    bool support_float64{};
    bool supports_denorm_behavior_independence{};
    bool supports_rounding_mode_independence{};
    bool support_fp16_denorm_preserve{};
    bool support_fp16_denorm_flush{};
    bool support_fp16_round_to_zero{};
    bool support_fp32_denorm_preserve{};
    bool support_fp32_denorm_flush{};
    bool support_fp32_round_to_zero{};
    bool support_fp64_denorm_preserve{};
    bool support_fp64_denorm_flush{};
    bool support_fp64_round_to_zero{};
    bool support_fp16_signed_zero_inf_nan_preserve{};
    bool support_fp32_signed_zero_inf_nan_preserve{};
    bool support_fp64_signed_zero_inf_nan_preserve{};
    bool supports_image_load_store_lod{};
    bool supports_native_cube_calc{};
    bool supports_trinary_minmax{};
    bool supports_buffer_fp32_atomic_min_max{};
    bool supports_image_fp32_atomic_min_max{};
    bool supports_buffer_int64_atomics{};
    bool supports_shared_int64_atomics{};
    bool supports_workgroup_explicit_memory_layout{};
    bool supports_amd_shader_explicit_vertex_parameter{};
    bool supports_fragment_shader_barycentric{};
    bool supports_shader_subgroup_clock{};
    bool has_broken_spirv_clamp{};
    bool lower_left_origin_mode{};
    bool needs_manual_interpolation{};
    /// port: NVIDIA without fragment shader barycentrics (Pascal and older).
    bool needs_integer_interpolation_fix{};
    bool needs_lds_barriers{};
    /// FEAT-005 Class D (HYPOTHESIS, needs AMD RX 9070 / #39 Chapter-5 capture to confirm the
    /// visual fix): the host cannot sample eA2B10G10R10SnormPack32, so GetSupportedFormat remapped
    /// the guest Snorm 2_10_10_10 surface to the unsigned eA2B10G10R10UintPack32 view. When set,
    /// the image-sample lowering reconstructs the signed [-1,1] decode in the shader
    /// (PatchImageSampleArgs). This is set on ANY host lacking the Snorm pack format, which
    /// includes the NVIDIA RTX 5060 repro machine - it is NOT NVIDIA-exempt. "No NVIDIA
    /// regression" is therefore a runtime claim to be verified by capture, not guaranteed by this
    /// gate.
    bool needs_snorm_2101010_expand{};
    bool needs_buffer_offsets{};
    bool needs_unorm_fixup{};
    bool needs_clip_distance_emulation{};
    bool needs_ssbo_store_load_barrier{};
    bool supports_shader_stencil_export{};
    /// port: THEORDER_LIGHTING_TEST=dref (vk_lighting_test.h): depth comparisons return the result
    /// in all four channels instead of (v, 0, 0, 0).
    bool dref_broadcast{};
    /// port: bump when translation changes the resources a shader declares for the same input,
    /// so a pipeline cache written by older code (whose layouts no longer match) is rebuilt.
    /// 1: stale/garbage T#s read as Image::Null (shader_recompiler/resource.h).
    /// 2: FMask reads keep their image and sampler declared (resource_patching_pass.cpp).
    /// 3: FMask read replacements return four components (they were invalid SPIR-V).
    /// 4: motion vertex shaders take the buffer addresses as specialization constants.
    u32 translation_revision{4};

    bool operator==(const Profile&) const = default;
};

} // namespace Shader
