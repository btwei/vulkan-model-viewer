// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include "vkmv/renderer/ResourceManager.hpp"

namespace vkmv {
    
void ResourceManager::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) {
    _instance = instance;
    _physicalDevice = physicalDevice;
    _device = device;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;


}

void ResourceManager::cleanup() {
    vmaDestroyAllocator(allocator);
}



} // namespace vkmv