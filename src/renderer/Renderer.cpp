// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <set>
#include <stdexcept>

#include "vkmv/renderer/Renderer.hpp"

namespace vkmv {

Renderer::Renderer(const Window& window)
: window(window) {
    initRenderer();
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::drawFrame(RenderableState& r) {

}

void Renderer::handleEvent(const SDL_Event& e){

}

void Renderer::initRenderer() {
    createInstance();
    createDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createDevice();
}

void Renderer::cleanup() {
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
    createInfo.enabledExtensionCount = enabledInstanceExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();

#ifndef NDEBUG
    // For non-release builds when the debug extension is present, enable a debug callback for instance creation and destruction
    if(std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != enabledInstanceExtensions.end()) return;
    
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
#endif

    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void Renderer::createDebugMessenger() {
#ifndef NDEBUG
    // If the extension is available
    if(std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != enabledInstanceExtensions.end()) return;

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
#endif
}

void Renderer::destroyDebugMessenger() {
#ifndef NDEBUG
    // If the extension is available
    if(std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != enabledInstanceExtensions.end()) return;

    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if(!vkDestroyDebugUtilsMessengerEXT) throw std::runtime_error("Failed to get proc address: vkDestroyDebugUtilsMessengerEXT!");

    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
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

    struct CandidateData {
        VkPhysicalDevice physicalDevice;
        bool qualified = true;
        std::vector<char*> candidateEnabledExtensions;
    };

    std::multimap<int, CandidateData> candidates;

    for(auto& device : physicalDevices) {
        CandidateData deviceTraits;
        int score;

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

                score += 500;
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

        // Prefer discrete GPUs (which tend to have better performance)
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        if(deviceTraits.qualified == true) candidates.insert(std::make_pair(score, deviceTraits));
    }

    if(candidates.rbegin()->first > 0){
        physicalDevice = candidates.rbegin()->second.physicalDevice;
        enabledDeviceExtensions = candidates.rbegin()->second.candidateEnabledExtensions;
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

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create device!");
    }

    vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamilyIndex, 0, &presentQueue);
}



} // namespace vkmv