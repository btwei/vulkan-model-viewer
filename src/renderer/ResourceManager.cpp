// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include "vkmv/renderer/ResourceManager.hpp"

#include <stdexcept>

namespace vkmv {
    
void ResourceManager::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) {
    _instance = instance;
    _physicalDevice = physicalDevice;
    _device = device;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

void ResourceManager::cleanup() {
    vmaDestroyAllocator(allocator);
}

VkImageAspectFlags getGenericAspectMask(VkFormat format) {
    if (format == VK_FORMAT_D16_UNORM ||
        format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
        format == VK_FORMAT_D32_SFLOAT) {
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    if (format == VK_FORMAT_S8_UINT) {
        return VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    return VK_IMAGE_ASPECT_COLOR_BIT;
}

AllocatedImage ResourceManager::allocateImage(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VkMemoryPropertyFlagBits properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
    AllocatedImage allocatedImage;

    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usageFlags;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.requiredFlags = properties;

    if(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if(vmaCreateImage(allocator, &createInfo, &allocCreateInfo, &allocatedImage.image, &allocatedImage.allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VkImage!");
    }

    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.image = allocatedImage.image;
    viewCreateInfo.format = format;

    viewCreateInfo.subresourceRange.aspectMask = getGenericAspectMask(format);
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    if(vkCreateImageView(_device, &viewCreateInfo, nullptr, &allocatedImage.imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }

    return allocatedImage;
}

void ResourceManager::destroyAllocatedImage(AllocatedImage allocatedImage) {
    vkDestroyImageView(_device, allocatedImage.imageView, nullptr);

    vmaDestroyImage(allocator, allocatedImage.image, allocatedImage.allocation);
}

} // namespace vkmv