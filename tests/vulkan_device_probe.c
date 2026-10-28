// SPDX-License-Identifier: GPL-2.0-or-later
// Headless CI preflight: a *real* Vulkan ICD is required for GPU tests.
// Exit 77 only when the driver is incompatible or no physical device exists.
// Unexpected loader/driver errors are failures, not silently skipped tests.
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

int main(void) {
    VkInstanceCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&info, NULL, &instance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        puts("Vulkan preflight: no compatible ICD available (skip code 77)");
        return 77;
    }
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Vulkan preflight: vkCreateInstance returned %d\n", (int)result);
        return 1;
    }
    uint32_t count = 0;
    result = vkEnumeratePhysicalDevices(instance, &count, NULL);
    vkDestroyInstance(instance, NULL);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Vulkan preflight: vkEnumeratePhysicalDevices returned %d\n",
                (int)result);
        return 1;
    }
    if (count == 0) {
        puts("Vulkan preflight: no physical devices available (skip code 77)");
        return 77;
    }
    printf("Vulkan preflight: %u physical device(s) available\n", (unsigned)count);
    return 0;
}
