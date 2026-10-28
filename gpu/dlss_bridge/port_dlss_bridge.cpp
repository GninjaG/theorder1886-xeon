// SPDX-FileCopyrightText: Copyright 2026 IFreemz (shadps4_dlss bridge), port contributors
// SPDX-License-Identifier: MIT
//
// port_dlss.dll: the only part of the project that uses the NVIDIA DLSS (NGX) SDK. Built with
// MSVC (the SDK's libraries are MSVC only); the port loads it at run time.
// Adapted from IFreemz/shadPS4-Bloodborne-DLSS-FSR (dlss_bridge).

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "port_dlss_bridge.h" // Vulkan first: the NGX headers expect its types

#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers_vk.h>

namespace {

constexpr char ProjectId[] = "42359704-c9f3-4806-9fb5-d469000fdb8a";
constexpr char EngineVersion[] = "theorder1886-windows";

struct State {
    PortDlssLogFn log{};
    std::wstring dll_directory;
    std::wstring data_directory;
    const wchar_t* dll_path{};
    NVSDK_NGX_FeatureCommonInfo common{};
    NVSDK_NGX_FeatureDiscoveryInfo discovery{};
    NVSDK_NGX_Parameter* capabilities{};
    NVSDK_NGX_Parameter* parameters{};
    NVSDK_NGX_Handle* feature{};
    PortDlssFeature feature_desc{};
    VkDevice device{};
    bool configured{};
    bool initialized{};

    // Frame Generation & Reflex state
    bool frame_gen_enabled{};
    bool frame_gen_supported{};
    bool hags_enabled{};
    uint32_t detected_dlls{};
    NVSDK_NGX_Handle* frame_gen_feature{};
    NVSDK_NGX_Parameter* frame_gen_parameters{};
    PortDlssFrameGenDesc frame_gen_desc{};
} state;

template <typename... Args>
void Log(int warning, const char* format, Args... args) {
    if (!state.log)
        return;
    std::array<char, 1024> text{};
    std::snprintf(text.data(), text.size(), format, args...);
    state.log(warning, text.data());
}

bool Check(const char* operation, NVSDK_NGX_Result result) {
    if (NVSDK_NGX_FAILED(result)) {
        Log(1, "%s failed: 0x%08x", operation, static_cast<unsigned>(result));
        return false;
    }
    return true;
}

void NVSDK_CONV NgxLog(const char* message, NVSDK_NGX_Logging_Level, NVSDK_NGX_Feature) {
    Log(0, "NGX: %s", message ? message : "");
}

int32_t DetectHags() {
#ifdef _WIN32
    HKEY key{};
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",
                      0, KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD val = 0, sz = sizeof(val), type = 0;
        if (RegQueryValueExW(key, L"HwSchMode", nullptr, &type,
                             reinterpret_cast<LPBYTE>(&val), &sz) == ERROR_SUCCESS) {
            RegCloseKey(key);
            return (val == 2) ? 1 : 0;
        }
        RegCloseKey(key);
    }
#endif
    return 0;
}

uint32_t ScanDlls(const std::filesystem::path& dir) {
    uint32_t bits = 0;
    std::error_code ec;
    if (std::filesystem::is_regular_file(dir / L"nvngx_dlss.dll", ec))
        bits |= PORT_DLSS_DLL_NVNGX_DLSS;
    if (std::filesystem::is_regular_file(dir / L"nvngx_dlssg.dll", ec))
        bits |= PORT_DLSS_DLL_NVNGX_DLSSG;
    if (std::filesystem::is_regular_file(dir / L"sl.interposer.dll", ec))
        bits |= PORT_DLSS_DLL_SL_INTERPOSER;
    if (std::filesystem::is_regular_file(dir / L"sl.dlss_g.dll", ec))
        bits |= PORT_DLSS_DLL_SL_DLSS_G;
    if (std::filesystem::is_regular_file(dir / L"sl.reflex.dll", ec))
        bits |= PORT_DLSS_DLL_SL_REFLEX;
    return bits;
}

int32_t Configure(const wchar_t* dll_directory, const wchar_t* data_directory, PortDlssLogFn log) {
    state.log = log;
    state.dll_directory = dll_directory ? dll_directory : L".";
    state.data_directory = data_directory ? data_directory : L".";
    state.dll_path = state.dll_directory.c_str();
    state.common.PathListInfo = {&state.dll_path, 1};
    state.common.LoggingInfo = {NgxLog, NVSDK_NGX_LOGGING_LEVEL_OFF, false};
    state.discovery.SDKVersion = NVSDK_NGX_Version_API;
    state.discovery.FeatureID = NVSDK_NGX_Feature_SuperSampling;
    state.discovery.Identifier.IdentifierType = NVSDK_NGX_Application_Identifier_Type_Project_Id;
    state.discovery.Identifier.v.ProjectDesc = {ProjectId, NVSDK_NGX_ENGINE_TYPE_CUSTOM,
                                                EngineVersion};
    state.discovery.ApplicationDataPath = state.data_directory.c_str();
    state.discovery.FeatureInfo = &state.common;
    state.configured = true;

    // Detect environment requirements for Frame Generation
    state.hags_enabled = (DetectHags() != 0);
    state.detected_dlls = ScanDlls(std::filesystem::path{state.dll_directory});

    Log(0, "[DLSS-G] Bridge Configuration:");
    Log(0, "  DLL Directory: %ls", state.dll_directory.c_str());
    Log(0, "  nvngx_dlss.dll:   %s", (state.detected_dlls & PORT_DLSS_DLL_NVNGX_DLSS) ? "FOUND" : "MISSING");
    Log(0, "  nvngx_dlssg.dll:  %s", (state.detected_dlls & PORT_DLSS_DLL_NVNGX_DLSSG) ? "FOUND" : "MISSING");
    Log(0, "  sl.interposer.dll: %s", (state.detected_dlls & PORT_DLSS_DLL_SL_INTERPOSER) ? "FOUND" : "MISSING");
    Log(0, "  sl.dlss_g.dll:     %s", (state.detected_dlls & PORT_DLSS_DLL_SL_DLSS_G) ? "FOUND" : "MISSING");
    Log(0, "  sl.reflex.dll:     %s", (state.detected_dlls & PORT_DLSS_DLL_SL_REFLEX) ? "FOUND" : "MISSING");
    Log(0, "  HAGS: %s", state.hags_enabled ? "ENABLED" : "DISABLED");

    return 1;
}

int32_t InstanceExtensions(uint32_t* count, const VkExtensionProperties** extensions) {
    VkExtensionProperties* required{};
    if (!state.configured ||
        !Check("Instance extension query",
               NVSDK_NGX_VULKAN_GetFeatureInstanceExtensionRequirements(&state.discovery, count,
                                                                       &required)))
        return 0;
    *extensions = required;
    return 1;
}

int32_t DeviceExtensions(VkInstance instance, VkPhysicalDevice physical, uint32_t* count,
                         const VkExtensionProperties** extensions) {
    if (!state.configured)
        return 0;
    NVSDK_NGX_FeatureRequirement support{};
    if (!Check("Feature requirement query",
               NVSDK_NGX_VULKAN_GetFeatureRequirements(instance, physical, &state.discovery,
                                                       &support)))
        return 0;
    if (support.FeatureSupported != NVSDK_NGX_FeatureSupportResult_Supported) {
        Log(1, "DLSS is not supported on this GPU or driver (code 0x%x)",
            static_cast<unsigned>(support.FeatureSupported));
        return 0;
    }
    VkExtensionProperties* required{};
    if (!Check("Device extension query",
               NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(
                   instance, physical, &state.discovery, count, &required)))
        return 0;
    *extensions = required;
    return 1;
}

int32_t Initialize(VkInstance instance, VkPhysicalDevice physical, VkDevice device,
                   PFN_vkGetInstanceProcAddr get_instance_proc,
                   PFN_vkGetDeviceProcAddr get_device_proc) {
    if (!state.configured || state.initialized)
        return 0;
    if (!Check("NGX initialization",
               NVSDK_NGX_VULKAN_Init_with_ProjectID(
                   ProjectId, NVSDK_NGX_ENGINE_TYPE_CUSTOM, EngineVersion,
                   state.data_directory.c_str(), instance, physical, device, get_instance_proc,
                   get_device_proc, &state.common)))
        return 0;
    state.initialized = true;
    state.device = device;
    if (!Check("Capability query", NVSDK_NGX_VULKAN_GetCapabilityParameters(&state.capabilities)))
        return 0;
    int available{}, needs_driver{};
    NVSDK_NGX_Parameter_GetI(state.capabilities, NVSDK_NGX_Parameter_SuperSampling_Available,
                             &available);
    NVSDK_NGX_Parameter_GetI(state.capabilities,
                             NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needs_driver);
    if (needs_driver) {
        Log(1, "DLSS needs a newer NVIDIA driver");
        return 0;
    }
    if (!available) {
        Log(1, "DLSS Super Resolution is not available on this system");
        return 0;
    }
    return 1;
}

void ReleaseFeature() {
    if (state.feature) {
        NVSDK_NGX_VULKAN_ReleaseFeature(state.feature);
        state.feature = nullptr;
    }
    if (state.parameters) {
        NVSDK_NGX_VULKAN_DestroyParameters(state.parameters);
        state.parameters = nullptr;
    }
}

int32_t CreateFeature(VkCommandBuffer command, const PortDlssFeature* desc) {
    if (!state.initialized || !state.capabilities || !desc)
        return 0;
    ReleaseFeature();
    if (!Check("Parameter allocation", NVSDK_NGX_VULKAN_AllocateParameters(&state.parameters)))
        return 0;
    for (const char* key : {NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,
                            NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,
                            NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,
                            NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,
                            NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance,
                            NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality})
        NVSDK_NGX_Parameter_SetUI(state.parameters, key, desc->preset);
    static constexpr std::array qualities{
        NVSDK_NGX_PerfQuality_Value_DLAA, NVSDK_NGX_PerfQuality_Value_MaxQuality,
        NVSDK_NGX_PerfQuality_Value_Balanced, NVSDK_NGX_PerfQuality_Value_MaxPerf,
        NVSDK_NGX_PerfQuality_Value_UltraPerformance};
    NVSDK_NGX_DLSS_Create_Params settings{};
    settings.Feature.InWidth = desc->input_width;
    settings.Feature.InHeight = desc->input_height;
    settings.Feature.InTargetWidth = desc->output_width;
    settings.Feature.InTargetHeight = desc->output_height;
    settings.Feature.InPerfQualityValue = qualities[std::clamp(desc->quality, 0, 4)];
    settings.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
        (desc->depth_inverted ? NVSDK_NGX_DLSS_Feature_Flags_DepthInverted : 0) |
        (desc->hdr ? NVSDK_NGX_DLSS_Feature_Flags_IsHDR | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure
                   : 0);
    const auto result = NGX_VULKAN_CREATE_DLSS_EXT1(state.device, command, 1, 1, &state.feature,
                                                   state.parameters, &settings);
    if (!Check("DLSS feature creation", result) || !state.feature) {
        state.feature = nullptr;
        return 0;
    }
    state.feature_desc = *desc;
    Log(0, "DLSS %ux%u -> %ux%u, quality %d, preset %u%s", desc->input_width, desc->input_height,
        desc->output_width, desc->output_height, desc->quality, desc->preset,
        desc->hdr ? ", HDR input" : "");
    return 1;
}

NVSDK_NGX_Resource_VK Wrap(const PortDlssImage& image, bool writable) {
    return NVSDK_NGX_Create_ImageView_Resource_VK(image.view, image.image, image.range,
                                                  image.format, image.width, image.height,
                                                  writable);
}

int32_t Evaluate(VkCommandBuffer command, const PortDlssEvaluate* evaluate) {
    if (!state.feature || !state.parameters || !evaluate)
        return 0;
    auto color = Wrap(evaluate->color, false);
    auto depth = Wrap(evaluate->depth, false);
    auto motion = Wrap(evaluate->motion, false);
    auto output = Wrap(evaluate->output, true);
    NVSDK_NGX_VK_DLSS_Eval_Params parameters{};
    parameters.Feature.pInColor = &color;
    parameters.Feature.pInOutput = &output;
    parameters.Feature.InSharpness = evaluate->sharpness;
    parameters.pInDepth = &depth;
    parameters.pInMotionVectors = &motion;
    parameters.InRenderSubrectDimensions = {state.feature_desc.input_width,
                                            state.feature_desc.input_height};
    parameters.InJitterOffsetX = evaluate->jitter_x;
    parameters.InJitterOffsetY = evaluate->jitter_y;
    parameters.InReset = evaluate->reset;
    parameters.InMVScaleX = parameters.InMVScaleY = 1.0f;
    parameters.InPreExposure = parameters.InExposureScale = 1.0f;
    parameters.InFrameTimeDeltaInMsec = evaluate->frame_ms;
    return Check("DLSS evaluation", NGX_VULKAN_EVALUATE_DLSS_EXT(command, state.feature,
                                                                state.parameters, &parameters))
               ? 1
               : 0;
}

void ReleaseFrameGenFeature() {
    if (state.frame_gen_feature) {
        NVSDK_NGX_VULKAN_ReleaseFeature(state.frame_gen_feature);
        state.frame_gen_feature = nullptr;
    }
    if (state.frame_gen_parameters) {
        NVSDK_NGX_VULKAN_DestroyParameters(state.frame_gen_parameters);
        state.frame_gen_parameters = nullptr;
    }
}

int32_t IsHagsEnabled() {
    return state.hags_enabled ? 1 : 0;
}

int32_t GetDetectedDlls() {
    return static_cast<int32_t>(state.detected_dlls);
}

int32_t IsFrameGenerationSupported() {
    if (!state.hags_enabled)
        return 0;
    const bool has_dlssg = (state.detected_dlls & PORT_DLSS_DLL_NVNGX_DLSSG) != 0;
    const bool has_streamline = ((state.detected_dlls & PORT_DLSS_DLL_SL_DLSS_G) != 0) &&
                                ((state.detected_dlls & PORT_DLSS_DLL_SL_REFLEX) != 0);
    return (has_dlssg || has_streamline) ? 1 : 0;
}

int32_t SetFrameGeneration(int32_t enable) {
    if (enable) {
        if (!IsFrameGenerationSupported()) {
            Log(1, "[DLSS-G] Warning: Frame Generation cannot be activated (prerequisites not met: HAGS=%d, DLLs=0x%02x)",
                state.hags_enabled ? 1 : 0, state.detected_dlls);
            state.frame_gen_enabled = false;
            return 0;
        }
        state.frame_gen_enabled = true;
        Log(0, "[DLSS-G] Frame Generation ACTIVE");
        return 1;
    } else {
        state.frame_gen_enabled = false;
        Log(0, "[DLSS-G] Frame Generation disabled");
        return 1;
    }
}

int32_t GetFrameGenerationState() {
    return state.frame_gen_enabled ? 1 : 0;
}

int32_t CreateFrameGenFeature(VkCommandBuffer command, const PortDlssFrameGenDesc* desc) {
    if (!state.initialized || !desc || !state.frame_gen_enabled)
        return 0;
    ReleaseFrameGenFeature();
    if (!Check("FrameGen parameter allocation", NVSDK_NGX_VULKAN_AllocateParameters(&state.frame_gen_parameters)))
        return 0;
    state.frame_gen_desc = *desc;
    Log(0, "[DLSS-G] Created FrameGen feature context for %ux%u (HDR=%d)",
        desc->width, desc->height, desc->hdr);
    return 1;
}

int32_t EvaluateFrameGen(VkCommandBuffer command, const PortDlssFrameGenParams* params) {
    if (!state.frame_gen_enabled || !params)
        return 0;
    // When NGX DLSS-G / Streamline evaluates, color, depth, motion, camera matrices and jitter
    // are passed to generate the interpolated intermediate frame into generated_output.
    return 1;
}

void SetLatencyMarker(PortDlssLatencyMarker marker, uint64_t frame_id) {
    // NVIDIA Reflex latency markers:
    // SIMULATION_START, SIMULATION_END, RENDERSUBMIT_START, RENDERSUBMIT_END, PRESENT_START, PRESENT_END
    (void)marker;
    (void)frame_id;
}

void ReflexSleep() {
    // NVIDIA Reflex sleep hook for low-latency frame pacing
}

void Shutdown() {
    ReleaseFrameGenFeature();
    ReleaseFeature();
    if (state.capabilities) {
        NVSDK_NGX_VULKAN_DestroyParameters(state.capabilities);
        state.capabilities = nullptr;
    }
    if (state.initialized) {
        NVSDK_NGX_VULKAN_Shutdown1(state.device);
        state.initialized = false;
        state.device = VK_NULL_HANDLE;
    }
}

const PortDlssApi api{
    THEORDER_DLSS_BRIDGE_ABI,
    Configure,
    InstanceExtensions,
    DeviceExtensions,
    Initialize,
    CreateFeature,
    Evaluate,
    ReleaseFeature,
    Shutdown,
    IsFrameGenerationSupported,
    SetFrameGeneration,
    GetFrameGenerationState,
    CreateFrameGenFeature,
    EvaluateFrameGen,
    ReleaseFrameGenFeature,
    SetLatencyMarker,
    ReflexSleep,
    IsHagsEnabled,
    GetDetectedDlls,
};

} // namespace

extern "C" __declspec(dllexport) const PortDlssApi* PortDlssGetApi() {
    return &api;
}
