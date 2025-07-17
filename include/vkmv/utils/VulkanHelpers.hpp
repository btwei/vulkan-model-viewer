// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_VULKANHELPERS_HPP
#define VKMV_VULKANHELPERS_HPP

#include <vulkan/vulkan.h>

namespace vkmv {

void transitionImageLayout(VkCommandBuffer buf, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

void blitImageToImage(VkCommandBuffer buf, VkImage src, VkImage dst, VkExtent3D srcSize, VkExtent3D dstSize);

} // namespace vkmv

#endif // VKMV_VULKANHELPERS_HPP