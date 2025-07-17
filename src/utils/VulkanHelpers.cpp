// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <iostream>
#include <stdexcept>

#include "vkmv/utils/VulkanHelpers.hpp"

namespace vkmv {

void transitionImageLayout(VkCommandBuffer buf, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    // Use an extremely permissive mask as a backup
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    // If a use case matches, swap in a more specific set of masks
    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        if(newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            barrier.srcAccessMask = VK_ACCESS_2_NONE;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }
    } else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        if(newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR;
            barrier.dstAccessMask = VK_ACCESS_2_NONE_KHR;
        }
    }

    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.image = image;

    barrier.subresourceRange.aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(buf, &depInfo);
}

void blitImageToImage(VkCommandBuffer buf, VkImage src, VkImage dst, VkExtent3D srcSize, VkExtent3D dstSize) {
    VkImageBlit2 blit{};
    blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcSubresource.mipLevel = 0;

    blit.srcOffsets[1].x = srcSize.width;
    blit.srcOffsets[1].y = srcSize.height;
    blit.srcOffsets[1].z = srcSize.depth;

    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstSubresource.mipLevel = 0;

    blit.dstOffsets[1].x = dstSize.width;
    blit.dstOffsets[1].y = dstSize.height;
    blit.dstOffsets[1].z = dstSize.depth;

    VkBlitImageInfo2 blitInfo{};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.srcImage = src;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.dstImage = dst;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blit;
    blitInfo.filter = VK_FILTER_LINEAR;

    vkCmdBlitImage2(buf, &blitInfo);
}

} // namespace vkmv