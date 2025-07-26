// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include "vkmv/renderer/Renderer.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <vk_mem_alloc.h>

#include "vkmv/utils/VulkanHelpers.hpp"

namespace vkmv {

Renderer::Renderer(const Window& window)
: window(window) {
    initRenderer();
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::handleEvent(const SDL_Event& e){

}

void Renderer::recordMainCommands(RenderableState& r, VkCommandBuffer& buf, VkImage& swapchainImage) {
    transitionImageLayout(buf, getCurrentFrame().renderTargetImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = getCurrentFrame().renderTargetImage.imageView;
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue.color = VkClearColorValue{0, 0, 0, 0};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = VkOffset2D{0 , 0};
    renderingInfo.renderArea.extent = VkExtent2D{width, height};
    renderingInfo.layerCount = 1;
    renderingInfo.viewMask = 0;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr; 

    vkCmdBeginRendering(buf, &renderingInfo);

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buf);

    vkCmdEndRendering(buf);

    transitionImageLayout(buf, getCurrentFrame().renderTargetImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    transitionImageLayout(buf, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    blitImageToImage(buf, getCurrentFrame().renderTargetImage.image, swapchainImage, VkExtent3D{width, height, 1}, VkExtent3D{width, height, 1});

    transitionImageLayout(buf, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void Renderer::drawFrame(RenderableState& r) {
    vkWaitForFences(device, 1, &getCurrentFrame().renderFence, VK_TRUE, 1'000'000'000);
    vkResetFences(device, 1, &getCurrentFrame().renderFence);

    uint32_t swapchainImageIndex;
    vkAcquireNextImageKHR(device, swapchain, 1'000'000'000, getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);

    VkCommandBuffer buf = getCurrentFrame().mainCommandBuffer;
    vkResetCommandBuffer(buf, 0);

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};
    vkBeginCommandBuffer(buf, &beginInfo);

        recordMainCommands(r, buf, swapchainImages[swapchainImageIndex]);

    vkEndCommandBuffer(buf);

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

    VkSemaphoreSubmitInfo waitSemaphoreInfo{};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = getCurrentFrame().swapchainSemaphore;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;

    VkSemaphoreSubmitInfo signalSemaphoreInfo{};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = swapchainImageResources[swapchainImageIndex].renderSemaphore;
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    VkCommandBufferSubmitInfo bufSubmitInfo{};
    bufSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    bufSubmitInfo.commandBuffer = buf;

    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &bufSubmitInfo;

    vkQueueSubmit2(graphicsQueue, 1, &submitInfo, getCurrentFrame().renderFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    presentInfo.pWaitSemaphores = &swapchainImageResources[swapchainImageIndex].renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    vkQueuePresentKHR(graphicsQueue, &presentInfo);

    frameCount++;
}

void Renderer::initRenderer() {
    Instance::create(&instance);
    createSurface();
    pickPhysicalDevice();
    createDevice();
    refreshWindowDims();
    createSwapchain();
    createCommandPools();
    createSyncObjects();
    resourceManager.init(instance.getInstance(), physicalDevice, device);
    createRenderTargets();
    initImGUI();
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(device);
    cleanupImGUI();
    destroyRenderTargets();
    resourceManager.cleanup();
    for(int i = 0; i < swapchainImageResources.size(); i++) vkDestroySemaphore(device, swapchainImageResources[i].renderSemaphore, nullptr);
    for(int i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(device, frames[i].commandPool, nullptr);

        vkDestroyFence(device, frames[i].renderFence, nullptr);
        vkDestroySemaphore(device, frames[i].swapchainSemaphore, nullptr);
    }
    for(int i = 0; i < swapchainImageViews.size(); i++) vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    SDL_Vulkan_DestroySurface(instance.getInstance(), surface, nullptr);
    Instance::destroy(&instance);
}

void Renderer::createSurface() {
    SDL_Vulkan_CreateSurface(window.getWindow(), instance.getInstance(), nullptr, &surface);
}

void Renderer::pickPhysicalDevice() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, physicalDevices.data());

    if(deviceCount == 0) throw std::runtime_error("Failed to gind GPUs with Vulkan support!");

    std::set<std::string> requiredDeviceExtensions;
    std::set<std::string> optionalDeviceExtensions;

    requiredDeviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    requiredDeviceExtensions.insert(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    requiredDeviceExtensions.insert(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    struct CandidateData {
        int score;
        VkPhysicalDevice physicalDevice;
        bool qualified = true;
        std::vector<std::string> candidateEnabledExtensions;

        bool operator<(const CandidateData& other) const {
            return score < other.score;
        }
    };

    std::priority_queue<CandidateData> candidates;

    for(auto& device : physicalDevices) {
        CandidateData deviceTraits;

        deviceTraits.physicalDevice = device;

        // GPU must support all required extensions
        // Prefer selecting GPUs with more desired optional extensions
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());\

        std::set<std::string> candidateRequiredExtensions(requiredDeviceExtensions);
        std::set<std::string> candidateOptionalExtensions(optionalDeviceExtensions);

        for(auto& extension : availableExtensions) {
            if(candidateRequiredExtensions.count(extension.extensionName) == 1) {
                candidateRequiredExtensions.erase(extension.extensionName);
                deviceTraits.candidateEnabledExtensions.push_back(extension.extensionName);

                continue;
            } else if(candidateOptionalExtensions.count(extension.extensionName) == 1) {
                deviceTraits.candidateEnabledExtensions.push_back(extension.extensionName);

                deviceTraits.score += 500;
            }
        }

        if(!candidateRequiredExtensions.empty()) {
            deviceTraits.qualified = false;
            continue;
        }

        // GPU must possess a queue family with graphics support and present support
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        bool graphicsFamilyFound = false;
        bool presentFamilyFound = false; 

        uint32_t i = 0;
        for(const auto& queueFamily : queueFamilies){
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamilyFound = true;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if(presentSupport) presentFamilyFound = true;

            i++;
        }

        if(!graphicsFamilyFound || !presentFamilyFound) {
            deviceTraits.qualified = false;
            continue;
        }

        // GPU must have at least one available format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if(formatCount == 0) {
            deviceTraits.qualified = false;
            continue;
        }

        // Prefer discrete GPUs (which tend to have better performance)
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            deviceTraits.score += 1000;
        }

        if(deviceTraits.qualified == true) candidates.push(deviceTraits);
    }

    if(candidates.top().score > 0){
        physicalDevice = candidates.top().physicalDevice;
        enabledDeviceExtensions = candidates.top().candidateEnabledExtensions;
    } else {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void Renderer::createDevice() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    bool graphicsFamilyFound = false;
    bool presentFamilyFound = false;

    // Find the queueFamilies for graphics and present support
    // It's likely that queue 0 will have support for both
    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if(!graphicsFamilyFound && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamilyIndex = i;
            graphicsFamilyFound = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        if(!presentFamilyFound && presentSupport) {
            presentFamilyIndex = i;
            presentFamilyFound = true;
        }

        i++;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { graphicsFamilyIndex, presentFamilyIndex };

    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> extensions;
    for(auto& string : enabledDeviceExtensions) {
        extensions.push_back(string.c_str());
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    VkPhysicalDeviceSynchronization2Features synchronization2{};
    synchronization2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
    dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRendering.dynamicRendering = VK_TRUE;
    dynamicRendering.pNext = &synchronization2;

    createInfo.pNext = &dynamicRendering;

    if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create device!");
    }

    vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamilyIndex, 0, &presentQueue);
}

void Renderer::createSwapchain() {
    // Select a Format
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    swapchainImageFormat = formats[0];
    for(const auto& format : formats) {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchainImageFormat = format;
            break;
        }
    }

    // Select a Present Mode
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(const auto& presentMode : presentModes) {
        if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = presentMode;
            break;
        }
    }

    // Query for surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps);

    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if(surfaceCaps.maxImageCount > 0 && imageCount > surfaceCaps.maxImageCount) {
        imageCount = surfaceCaps.maxImageCount;
    }

    // Select an extent 
    if(surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapchainExtent = surfaceCaps.currentExtent;
    } else {
        swapchainExtent = {std::clamp(static_cast<uint32_t>(width), surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
                           std::clamp(static_cast<uint32_t>(height), surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height)};
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapchainImageFormat.format;
    createInfo.imageColorSpace = swapchainImageFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t queueFamilyIndices[] = {graphicsFamilyIndex, presentFamilyIndex};

    if(graphicsFamilyIndex != presentFamilyIndex){
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = surfaceCaps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = swapchainPresentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain!");
    }

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    swapchainImageViews.resize(imageCount);
    for(size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = swapchainImages[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = swapchainImageFormat.format;

        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device, &viewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain image view!");
        }
    }
}

void Renderer::destroySwapchain() {
    for(int i = 0; i < swapchainImageViews.size(); i++) vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::createCommandPools() {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = graphicsFamilyIndex;

    for(int i=0; i<NUM_FRAMES_IN_FLIGHT; i++) {
        if(vkCreateCommandPool(device, &createInfo, nullptr, &frames[i].commandPool) != VK_SUCCESS){
            throw std::runtime_error("Failed to create command pool!");
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = frames[i].commandPool;
        allocInfo.commandBufferCount = 1;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].mainCommandBuffer) != VK_SUCCESS){
            throw std::runtime_error("Failed to allocate main command buffer!");
        }
    }
}

void Renderer::createSyncObjects() {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    for(int i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        if(vkCreateFence(device, &fenceInfo, nullptr, &frames[i].renderFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence!");
        }

        if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frames[i].swapchainSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore!");
        }
    }

    swapchainImageResources.resize(swapchainImages.size());
    for(int i=0; i < swapchainImages.size(); i++) {
        if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &swapchainImageResources[i].renderSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore!");
        }
    }
}

/**
 * @todo Move window size logic into the window class
*/ 
void Renderer::createRenderTargets() {
    int w, h;
    SDL_GetWindowSizeInPixels(window.getWindow(), &w, &h);

    unsigned int width = static_cast<unsigned int>(w);
    unsigned int height = static_cast<unsigned int>(h);

    for(int i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        frames[i].renderTargetImage = resourceManager.allocateImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VkExtent3D{width, height, 1}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
}

void Renderer::destroyRenderTargets() {
    for(int i= 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        resourceManager.destroyAllocatedImage(frames[i].renderTargetImage);
    }
}

Renderer::FrameData& Renderer::getCurrentFrame() {
    return frames[frameCount % NUM_FRAMES_IN_FLIGHT];
}

void Renderer::refreshWindowDims() {
    int w, h;
    SDL_GetWindowSizeInPixels(window.getWindow(), &w, &h);

    width = w;
    height = h;
}

void Renderer::initImGUI() {
    VkFormat colorAttachmentFormats[] = { VK_FORMAT_R16G16B16A16_SFLOAT };

    VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderingCreateInfo.pNext = nullptr;
    renderingCreateInfo.viewMask = 0;
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;
    renderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForVulkan(window.getWindow());
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_VERSION_1_3;
    init_info.Instance = instance.getInstance();
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = graphicsFamilyIndex;
    init_info.Queue = graphicsQueue;
    init_info.DescriptorPool = VK_NULL_HANDLE;
    init_info.RenderPass = VK_NULL_HANDLE;

    init_info.MinImageCount = swapchainImages.size();
    init_info.ImageCount = swapchainImages.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.Subpass = 0;

    init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;

    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = renderingCreateInfo;

    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    init_info.MinAllocationSize = 1024*1024;

    ImGui_ImplVulkan_Init(&init_info);
}

void Renderer::cleanupImGUI() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

} // namespace vkmv