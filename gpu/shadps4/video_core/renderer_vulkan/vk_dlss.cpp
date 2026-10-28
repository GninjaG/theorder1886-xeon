// SPDX-FileCopyrightText: Copyright 2026 IFreemz, port contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string_view>

#include "video_core/renderer_vulkan/vk_dlss.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "../../../dlss_bridge/port_dlss_bridge.h"
#endif

namespace Vulkan {

int Dlss::QualityForScale(float scale) {
    // DLAA only for equal sizes: any real upscale needs at least the Quality mode.
    return scale >= 2.9f ? 4 : scale >= 1.95f ? 3 : scale >= 1.65f ? 2 : scale > 1.01f ? 1 : 0;
}

#ifdef _WIN32

namespace {
constexpr wchar_t BridgeName[] = L"port_dlss.dll";
constexpr wchar_t NgxName[] = L"nvngx_dlss.dll";

std::filesystem::path ExecutableDirectory() {
    std::wstring path(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
    path.resize(length);
    return std::filesystem::path{path}.parent_path();
}

std::filesystem::path FindDllDirectory() {
    const auto exe_dir = ExecutableDirectory();
    std::error_code ec;
    const std::array candidates = {
        exe_dir,
        exe_dir / "out",
        std::filesystem::current_path(ec),
        std::filesystem::current_path(ec) / "out",
    };
    for (const auto& c : candidates) {
        if (!c.empty() && std::filesystem::is_regular_file(c / BridgeName, ec)) {
            return c;
        }
    }
    return exe_dir;
}

bool CheckHags() {
    HKEY key{};
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",
                      0, KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD val = 0, sz = sizeof(val), type = 0;
        if (RegQueryValueExW(key, L"HwSchMode", nullptr, &type,
                             reinterpret_cast<LPBYTE>(&val), &sz) == ERROR_SUCCESS) {
            RegCloseKey(key);
            return (val == 2);
        }
        RegCloseKey(key);
    }
    return false;
}

u32 ScanDlls(const std::filesystem::path& dir) {
    u32 bits = 0;
    std::error_code ec;
    if (std::filesystem::is_regular_file(dir / "nvngx_dlss.dll", ec))
        bits |= PORT_DLSS_DLL_NVNGX_DLSS;
    if (std::filesystem::is_regular_file(dir / "nvngx_dlssg.dll", ec))
        bits |= PORT_DLSS_DLL_NVNGX_DLSSG;
    if (std::filesystem::is_regular_file(dir / "sl.interposer.dll", ec))
        bits |= PORT_DLSS_DLL_SL_INTERPOSER;
    if (std::filesystem::is_regular_file(dir / "sl.dlss_g.dll", ec))
        bits |= PORT_DLSS_DLL_SL_DLSS_G;
    if (std::filesystem::is_regular_file(dir / "sl.reflex.dll", ec))
        bits |= PORT_DLSS_DLL_SL_REFLEX;
    return bits;
}

void BridgeLog(int warning, const char* message) {
    std::printf("DLSS: %s%s\n", warning ? "warning: " : "", message);
}

bool CollectExtensions(const VkExtensionProperties* required, u32 count,
                       const std::vector<vk::ExtensionProperties>& available,
                       std::vector<std::string>& storage, std::vector<const char*>& enabled) {
    storage.clear();
    for (u32 i = 0; i < count; ++i) {
        const auto& request = required[i];
        const auto match = std::ranges::find_if(available, [&](const auto& extension) {
            return std::strcmp(extension.extensionName.data(), request.extensionName) == 0 &&
                   extension.specVersion >= request.specVersion;
        });
        if (match == available.end()) {
            std::printf("DLSS: missing Vulkan extension %s\n", request.extensionName);
            return false;
        }
        storage.emplace_back(request.extensionName);
    }
    for (const auto& name : storage) {
        if (std::ranges::none_of(enabled, [&](const char* current) { return name == current; })) {
            enabled.push_back(name.c_str());
        }
    }
    return true;
}
} // namespace

struct Dlss::Impl {
    HMODULE module{};
    const PortDlssApi* api{};
    std::vector<std::string> instance_extensions, device_extensions;
    std::optional<FeatureDesc> feature;
    std::string problem;
    std::string frame_gen_problem;
    bool eligible{true}, instance_ready{}, device_ready{}, initialized{}, available{};
    bool hags_enabled{false};
    bool is_ada_or_newer{false};
    bool frame_gen_active{false};
    bool frame_gen_enabled_user{false};
    u32 detected_dlls{0};
    std::filesystem::path dll_dir;
    std::string gpu_name;

    void Disable(std::string_view reason) {
        eligible = available = false;
        problem = reason;
        std::printf("DLSS: unavailable (%.*s)\n", int(reason.size()), reason.data());
    }
};

Dlss* Dlss::Get() {
    static Dlss* const dlss = []() -> Dlss* {
        const char* setting = std::getenv("THEORDER_DLSS");
        if (setting && setting[0] == '0') {
            return nullptr;
        }
        const auto directory = FindDllDirectory();
        if (!std::filesystem::is_regular_file(directory / BridgeName) ||
            !std::filesystem::is_regular_file(directory / NgxName)) {
            return nullptr;
        }
        auto* created = new Dlss;
        // The process may end through quick_exit/_exit paths: release NGX first when it can.
        std::atexit([] {
            if (Dlss* live = Get()) {
                live->Shutdown();
            }
        });
        return created;
    }();
    return dlss;
}

Dlss::Dlss() : impl{std::make_unique<Impl>()} {
    const auto directory = FindDllDirectory();
    impl->dll_dir = directory;
    impl->hags_enabled = CheckHags();
    impl->detected_dlls = ScanDlls(directory);

    impl->module = LoadLibraryW((directory / BridgeName).c_str());
    const auto get_api =
        impl->module ? reinterpret_cast<PortDlssGetApiFn>(GetProcAddress(impl->module, "PortDlssGetApi"))
                     : nullptr;
    impl->api = get_api ? get_api() : nullptr;
    if (!impl->api || (impl->api->abi != THEORDER_DLSS_BRIDGE_ABI && impl->api->abi != 1)) {
        impl->Disable("port_dlss.dll is missing or from another version");
        return;
    }
    // NGX writes its logs and model updates here: beside the saves and shader caches.
    std::filesystem::path data = directory / "dlss";
    if (const char* user = std::getenv("THEORDER_GPU_USER_DIR"); user && user[0]) {
        data = std::filesystem::path{user} / "dlss";
    }
    std::error_code error;
    std::filesystem::create_directories(data, error);
    if (!impl->api->Configure(directory.c_str(), data.c_str(), BridgeLog)) {
        impl->Disable("bridge configuration failed");
    }
}

Dlss::~Dlss() {
    Shutdown();
}

void Dlss::AppendInstanceExtensions(std::vector<const char*>& enabled) {
    if (!impl->eligible) {
        return;
    }
    u32 count{};
    const VkExtensionProperties* required{};
    if (!impl->api->InstanceExtensions(&count, &required)) {
        return impl->Disable("instance extension query failed");
    }
    const auto [result, extensions] = vk::enumerateInstanceExtensionProperties();
    if (result != vk::Result::eSuccess ||
        !CollectExtensions(required, count, extensions, impl->instance_extensions, enabled)) {
        return impl->Disable("required Vulkan instance extensions are missing");
    }
    impl->instance_ready = true;
}

void Dlss::AppendDeviceExtensions(vk::Instance instance, vk::PhysicalDevice physical,
                                  std::vector<const char*>& enabled) {
    if (!impl->eligible || !impl->instance_ready) {
        return;
    }
    const auto properties = physical.getProperties();
    if (properties.vendorID != 0x10de) {
        return impl->Disable("not an NVIDIA GPU");
    }
    u32 count{};
    const VkExtensionProperties* required{};
    if (!impl->api->DeviceExtensions(instance, physical, &count, &required)) {
        return impl->Disable("this GPU or driver does not support DLSS (GeForce RTX needed)");
    }
    const auto [result, extensions] = physical.enumerateDeviceExtensionProperties();
    if (result != vk::Result::eSuccess ||
        !CollectExtensions(required, count, extensions, impl->device_extensions, enabled)) {
        return impl->Disable("required Vulkan device extensions are missing");
    }
    impl->device_ready = true;
}

void Dlss::Disable(const std::string& reason) {
    impl->Disable(reason);
}

void Dlss::Initialize(vk::Instance instance, vk::PhysicalDevice physical, vk::Device device) {
    if (!impl->eligible || !impl->device_ready || impl->initialized) {
        return;
    }
    const auto props = physical.getProperties();
    impl->gpu_name = props.deviceName.data();
    const std::string name_lower = [&] {
        std::string s = impl->gpu_name;
        for (char& c : s) c = static_cast<char>(std::tolower(c));
        return s;
    }();
    impl->is_ada_or_newer = (props.deviceID >= 0x2600) ||
                            (name_lower.find("rtx 40") != std::string::npos) ||
                            (name_lower.find("rtx 50") != std::string::npos) ||
                            (name_lower.find("ada") != std::string::npos) ||
                            (name_lower.find("blackwell") != std::string::npos);

    impl->initialized = true;
    if (!impl->api->Initialize(instance, physical, device,
                               VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
                               VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr)) {
        return impl->Disable("NVIDIA NGX initialization failed (update the GPU driver)");
    }
    impl->available = true;
    std::printf("DLSS: ready\n");

    // Complete diagnostic logging
    std::printf("[DLSS-G] Diagnostic Check:\n");
    std::printf("  nvngx_dlss.dll:   %s\n", (impl->detected_dlls & PORT_DLSS_DLL_NVNGX_DLSS) ? "FOUND" : "MISSING");
    std::printf("  nvngx_dlssg.dll:  %s\n", (impl->detected_dlls & PORT_DLSS_DLL_NVNGX_DLSSG) ? "FOUND" : "MISSING");
    std::printf("  sl.interposer.dll: %s\n", (impl->detected_dlls & PORT_DLSS_DLL_SL_INTERPOSER) ? "FOUND" : "MISSING");
    std::printf("  sl.dlss_g.dll:     %s\n", (impl->detected_dlls & PORT_DLSS_DLL_SL_DLSS_G) ? "FOUND" : "MISSING");
    std::printf("  sl.reflex.dll:     %s\n", (impl->detected_dlls & PORT_DLSS_DLL_SL_REFLEX) ? "FOUND" : "MISSING");
    std::printf("  Hardware-Accelerated GPU Scheduling (HAGS): %s\n", impl->hags_enabled ? "ENABLED" : "DISABLED");
    std::printf("  GPU Architecture: %s (Optical Flow: %s)\n", impl->gpu_name.c_str(),
                impl->is_ada_or_newer ? "SUPPORTED (Ada Lovelace+)" : "UNSUPPORTED (Pre-Ada / No OFA)");

    const char* fg_env = std::getenv("THEORDER_FRAME_GEN");
    const char* dlssg_env = std::getenv("THEORDER_DLSS_G");
    if ((fg_env && fg_env[0] != '0') || (dlssg_env && dlssg_env[0] != '0')) {
        SetNvidiaFrameGeneration(true);
    }
}

bool Dlss::Available() const {
    return impl->available;
}

std::string Dlss::Problem() const {
    return impl->problem;
}

void Dlss::SetNvidiaFrameGeneration(bool enable) {
    if (!impl) return;
    impl->frame_gen_enabled_user = enable;
    if (!enable) {
        impl->frame_gen_active = false;
        if (impl->api && impl->api->abi >= 2 && impl->api->SetFrameGeneration) {
            impl->api->SetFrameGeneration(0);
        }
        std::printf("[DLSS-G] Frame Generation disabled\n");
        return;
    }

    std::vector<std::string> missing;
    if (!impl->is_ada_or_newer) {
        missing.push_back("GeForce RTX 40 series or newer required (Ada Lovelace+) - current: " + impl->gpu_name);
    }
    if (!impl->hags_enabled) {
        missing.push_back("Hardware-Accelerated GPU Scheduling (HAGS) must be enabled in Windows Settings");
    }
    const bool has_dlssg = (impl->detected_dlls & PORT_DLSS_DLL_NVNGX_DLSSG) != 0;
    const bool has_streamline = ((impl->detected_dlls & PORT_DLSS_DLL_SL_DLSS_G) != 0) &&
                                ((impl->detected_dlls & PORT_DLSS_DLL_SL_REFLEX) != 0);
    if (!has_dlssg && !has_streamline) {
        missing.push_back("Missing DLSS-G / Reflex DLLs (requires nvngx_dlssg.dll or sl.dlss_g.dll + sl.reflex.dll)");
    }

    if (!missing.empty()) {
        std::string reason;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) reason += "; ";
            reason += missing[i];
        }
        impl->frame_gen_problem = reason;
        impl->frame_gen_active = false;
        std::printf("[DLSS-G] Warning: Requirements not met: %s\n", reason.c_str());
        if (impl->api && impl->api->abi >= 2 && impl->api->SetFrameGeneration) {
            impl->api->SetFrameGeneration(0);
        }
        return;
    }

    impl->frame_gen_problem.clear();
    impl->frame_gen_active = true;
    if (impl->api && impl->api->abi >= 2 && impl->api->SetFrameGeneration) {
        impl->api->SetFrameGeneration(1);
    }
    std::printf("[DLSS-G] Frame Generation ACTIVE\n");
}

bool Dlss::IsFrameGenerationEnabled() const {
    return impl ? impl->frame_gen_enabled_user : false;
}

bool Dlss::IsFrameGenerationActive() const {
    return impl ? impl->frame_gen_active : false;
}

bool Dlss::IsFrameGenerationSupported() const {
    if (!impl) return false;
    const bool has_dlssg = (impl->detected_dlls & PORT_DLSS_DLL_NVNGX_DLSSG) != 0;
    const bool has_streamline = ((impl->detected_dlls & PORT_DLSS_DLL_SL_DLSS_G) != 0) &&
                                ((impl->detected_dlls & PORT_DLSS_DLL_SL_REFLEX) != 0);
    return impl->is_ada_or_newer && impl->hags_enabled && (has_dlssg || has_streamline);
}

std::string Dlss::FrameGenProblem() const {
    return impl ? impl->frame_gen_problem : "";
}

void Dlss::SetLatencyMarker(PortDlssLatencyMarker marker, u64 frame_id) {
    if (impl && impl->api && impl->api->abi >= 2 && impl->api->SetLatencyMarker) {
        impl->api->SetLatencyMarker(marker, frame_id);
    }
}

void Dlss::ReflexSleep() {
    if (impl && impl->api && impl->api->abi >= 2 && impl->api->ReflexSleep) {
        impl->api->ReflexSleep();
    }
}

bool Dlss::IsHagsEnabled() const {
    return impl ? impl->hags_enabled : false;
}

u32 Dlss::GetDetectedDlls() const {
    return impl ? impl->detected_dlls : 0;
}

bool Dlss::CreateFrameGenFeature(vk::CommandBuffer command, u32 width, u32 height, vk::Format format, bool hdr) {
    if (!impl || !impl->frame_gen_active || !impl->api)
        return false;
    if (impl->api->abi >= 2 && impl->api->CreateFrameGenFeature) {
        const PortDlssFrameGenDesc desc{width, height, static_cast<VkFormat>(format), hdr ? 1 : 0, 0};
        return impl->api->CreateFrameGenFeature(command, &desc) != 0;
    }
    return true;
}

bool Dlss::EvaluateFrameGen(vk::CommandBuffer command, const FrameGenData& data) {
    if (!impl || !impl->frame_gen_active || !impl->api)
        return false;
    if (impl->api->abi >= 2 && impl->api->EvaluateFrameGen) {
        const auto wrap = [](const Resource& r, bool writable) {
            const vk::ImageSubresourceRange range{r.aspect, 0, 1, 0, 1};
            return PortDlssImage{r.image, r.view, static_cast<VkImageSubresourceRange>(range),
                                 static_cast<VkFormat>(r.format), r.width, r.height};
        };
        const PortDlssFrameGenParams params{
            .color = wrap(data.color, false),
            .depth = wrap(data.depth, false),
            .motion = wrap(data.motion, false),
            .hudless = wrap(data.hudless.image ? data.hudless : data.color, false),
            .generated_output = wrap(data.output, true),
            .camera = data.camera,
            .frame_id = data.frame_id,
            .reset = data.reset ? 1 : 0,
        };
        return impl->api->EvaluateFrameGen(command, &params) != 0;
    }
    return true;
}

void Dlss::ReleaseFrameGenFeature() {
    if (impl && impl->api && impl->api->abi >= 2 && impl->api->ReleaseFrameGenFeature) {
        impl->api->ReleaseFrameGenFeature();
    }
}

bool Dlss::HasFeature(const FeatureDesc& desc) const {
    return impl->feature && *impl->feature == desc;
}

bool Dlss::CreateFeature(vk::CommandBuffer command, const FeatureDesc& desc) {
    if (!impl->available) {
        return false;
    }
    impl->feature.reset();
    const PortDlssFeature feature{desc.input_width,  desc.input_height, desc.output_width,
                                desc.output_height, desc.quality,      0,
                                0,                  desc.hdr ? 1 : 0};
    if (!impl->api->CreateFeature(command, &feature)) {
        return false;
    }
    impl->feature = desc;
    return true;
}

bool Dlss::Evaluate(vk::CommandBuffer command, const Frame& frame) {
    if (!impl->available || !impl->feature) {
        return false;
    }
    const auto image = [](const Resource& r) {
        const vk::ImageSubresourceRange range{r.aspect, 0, 1, 0, 1};
        return PortDlssImage{r.image,
                           r.view,
                           static_cast<VkImageSubresourceRange>(range),
                           static_cast<VkFormat>(r.format),
                           r.width,
                           r.height};
    };
    const PortDlssEvaluate parameters{image(frame.color),  image(frame.depth), image(frame.motion),
                                    image(frame.output), frame.jitter_x,     frame.jitter_y,
                                    frame.reset ? 1 : 0, frame.frame_ms,     frame.sharpness};
    return impl->api->Evaluate(command, &parameters) != 0;
}

void Dlss::ReleaseFeature() {
    impl->feature.reset();
    if (impl->api) {
        impl->api->ReleaseFeature();
    }
}

void Dlss::Shutdown() {
    impl->available = false;
    impl->feature.reset();
    if (impl->api && impl->initialized) {
        impl->api->Shutdown();
        impl->initialized = false;
    }
}

#else

struct Dlss::Impl {};
Dlss::Dlss() = default;
Dlss::~Dlss() = default;
Dlss* Dlss::Get() {
    return nullptr;
}
void Dlss::AppendInstanceExtensions(std::vector<const char*>&) {}
void Dlss::AppendDeviceExtensions(vk::Instance, vk::PhysicalDevice, std::vector<const char*>&) {}
void Dlss::Initialize(vk::Instance, vk::PhysicalDevice, vk::Device) {}
void Dlss::Disable(const std::string&) {}
void Dlss::Shutdown() {}
bool Dlss::Available() const {
    return false;
}
std::string Dlss::Problem() const {
    return {};
}
bool Dlss::HasFeature(const FeatureDesc&) const {
    return false;
}
bool Dlss::CreateFeature(vk::CommandBuffer, const FeatureDesc&) {
    return false;
}
bool Dlss::Evaluate(vk::CommandBuffer, const Frame&) {
    return false;
}
void Dlss::ReleaseFeature() {}
void Dlss::SetNvidiaFrameGeneration(bool) {}
bool Dlss::IsFrameGenerationEnabled() const { return false; }
bool Dlss::IsFrameGenerationActive() const { return false; }
bool Dlss::IsFrameGenerationSupported() const { return false; }
std::string Dlss::FrameGenProblem() const { return {}; }
void Dlss::SetLatencyMarker(PortDlssLatencyMarker, u64) {}
void Dlss::ReflexSleep() {}
bool Dlss::IsHagsEnabled() const { return false; }
u32 Dlss::GetDetectedDlls() const { return 0; }
bool Dlss::CreateFrameGenFeature(vk::CommandBuffer, u32, u32, vk::Format, bool) { return false; }
bool Dlss::EvaluateFrameGen(vk::CommandBuffer, const FrameGenData&) { return false; }
void Dlss::ReleaseFrameGenFeature() {}

#endif

} // namespace Vulkan
