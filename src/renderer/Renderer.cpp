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

void Renderer::recordMainCommands(RenderableState& r, VkCommandBuffer& buf, VkImage& swapchainImage) {
    transitionImageLayout(buf, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    transitionImageLayout(buf, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void Renderer::handleEvent(const SDL_Event& e){

}

void Renderer::initRenderer() {
    createInstance();
    createDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createDevice();
    createSwapchain();
    createCommandPools();
    createSyncObjects();
    resourceManager.init(instance, physicalDevice, device);
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(device);

    for(int i = 0; i < swapchainImageResources.size(); i++) vkDestroySemaphore(device, swapchainImageResources[i].renderSemaphore, nullptr);
    for(int i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(device, frames[i].commandPool, nullptr);

        vkDestroyFence(device, frames[i].renderFence, nullptr);
        vkDestroySemaphore(device, frames[i].swapchainSemaphore, nullptr);
    }
    for(int i = 0; i < swapchainImageViews.size(); i++) vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    destroyDebugMessenger();
    vkDestroyInstance(instance, nullptr);
}

// Callback to handle validation layer debug messages
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "Khronos Validation Layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void Renderer::createInstance() {
    std::set<std::string> requiredLayers;
    std::set<std::string> optionalLayers; // duplicate entries are allowed between required and optional

    std::set<std::string> requiredInstanceExtensions;
    std::set<std::string> optionalInstanceExtensions; // duplicate entries are allowed between required and optional

    // Populate required, optional layers and extensions
    uint32_t surfaceExtensionCount;
    const char* const *surfaceExtensions = SDL_Vulkan_GetInstanceExtensions(&surfaceExtensionCount);

    if(surfaceExtensions == nullptr) throw std::runtime_error("Failed to get SDL Vulkan instance extensions!");

    for(int ext = 0; ext < surfaceExtensionCount; ext++) {
        requiredInstanceExtensions.insert(surfaceExtensions[ext]);
    }

    // If debug, add validation layers if available
    #ifndef NDEBUG
        optionalLayers.insert("VK_LAYER_KHRONOS_validation");
        optionalInstanceExtensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    // Query against available, throw if required is not available
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(auto& layer : availableLayers) {
        if(requiredLayers.count(layer.layerName) == 1) {
            requiredLayers.erase(layer.layerName);
            enabledInstanceLayers.push_back(layer.layerName);

            continue;
        } else if(optionalLayers.count(layer.layerName) == 1) {
            enabledInstanceLayers.push_back(layer.layerName);
        }
    }

    if(!requiredLayers.empty()) {
        for(auto& layer : requiredLayers) {
            std::cerr << "Missing required layer: " << layer << std::endl;
        }
        throw std::runtime_error("Missing one or more required layers!");
    }

    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    for(auto& extension : availableExtensions) {
        if(requiredInstanceExtensions.count(extension.extensionName) == 1) {
            requiredInstanceExtensions.erase(extension.extensionName);
            enabledInstanceExtensions.push_back(extension.extensionName);

            continue;
        } else if(optionalInstanceExtensions.count(extension.extensionName) == 1) {
            enabledInstanceExtensions.push_back(extension.extensionName);
        }
    }
    
    if(!requiredInstanceExtensions.empty()) {
        for(auto& extension : requiredInstanceExtensions) {
            std::cerr << "Missing required extension: " << extension << std::endl;
        }
        throw std::runtime_error("Missing one or more required instance extensions!");
    }

    std::vector<const char*> extensions;
    for(auto& ext : enabledInstanceExtensions) {
        extensions.push_back(ext.c_str());
    }

    // Create Instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Model Viewer";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = enabledInstanceLayers.size();
    createInfo.ppEnabledLayerNames = enabledInstanceLayers.data();
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    // For non-release builds when the debug extension is present, enable a debug callback for instance creation and destruction
    if(std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), std::string(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) != enabledInstanceExtensions.end()) {
    
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = vkmv::debugCallback;

        createInfo.pNext = &debugCreateInfo;
    }
#endif

    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void Renderer::createDebugMessenger() {
#ifndef NDEBUG
    // If the extension is available
    if(std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != enabledInstanceExtensions.end()) {

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = vkmv::debugCallback;

        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); // alternatively, use Volk to load functions
        if(!vkCreateDebugUtilsMessengerEXT) throw std::runtime_error("Failed to get ProcAddr: vkCreateDebugUtilsMessengerEXT!");

        if(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create debug messenger!");
        }
    }
#endif
}

void Renderer::destroyDebugMessenger() {
#ifndef NDEBUG
    // If the extension is available
    if(std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != enabledInstanceExtensions.end()) {

        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(!vkDestroyDebugUtilsMessengerEXT) throw std::runtime_error("Failed to get proc address: vkDestroyDebugUtilsMessengerEXT!");
        
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
#endif
}

void Renderer::createSurface() {
    SDL_Vulkan_CreateSurface(window.getWindow(), instance, nullptr, &surface);
}

void Renderer::pickPhysicalDevice() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

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
        int w, h;
        SDL_GetWindowSizeInPixels(window.getWindow(), &w, &h);

        swapchainExtent = {std::clamp(static_cast<uint32_t>(w), surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
                           std::clamp(static_cast<uint32_t>(h), surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height)};
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

Renderer::FrameData& Renderer::getCurrentFrame() {
    return frames[frameCount % NUM_FRAMES_IN_FLIGHT];
}

} // namespace vkmv