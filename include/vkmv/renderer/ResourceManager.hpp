// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_RESOURCEMANAGER_HPP
#define VKMV_RESOURCEMANAGER_HPP

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

namespace vkmv {

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

/**
 * @class ResourceManager
 * @brief
 * 
 * 
 */
class ResourceManager {
public:
    /**
     * @brief Initializes this class. Must be called before calling another other ResourceManager functions.
     */
    void init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

    /**
     * @brief Cleans up this class. No calls may be used again after calling cleanup.
     */
    void cleanup();

    AllocatedImage allocateImage(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VkMemoryPropertyFlagBits memoryProperties);

    void destroyAllocatedImage(AllocatedImage allocatedImage);

private:
    VkInstance _instance;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;

    VmaAllocator allocator;

};

} // namespace vkmv

#endif // VKMV_RESOURCEMANAGER_HPP