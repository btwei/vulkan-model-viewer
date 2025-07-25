// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "SDL3/SDL_vulkan.h"
#include "vkmv/core/Instance.hpp"

namespace vkmv {

void Instance::create(Instance* pInstance) {
    createInstance(pInstance);
    createDebugMessenger(pInstance);
}

void Instance::destroy(Instance* pInstance) {
    destroyDebugMessenger(pInstance);
    vkDestroyInstance(pInstance->_instance, nullptr);
}

bool Instance::isExtensionEnabled(const char* extension) const {
    return std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), std::string(extension)) != enabledInstanceExtensions.end();
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

/**
 * @brief Creates an instance with the requested layers and extensions.
 * 
 * @note This was copied from an older piece of code and could use some cleanup.
 */
void Instance::createInstance(Instance* pInstance) {

    // Allow optional and required layers and extensions to be specified
    // Currently none are explitly added, and this structure is for future use

    std::set<std::string> requiredLayers;
    std::set<std::string> optionalLayers; // duplicate entries are allowed between required and optional

    std::set<std::string> requiredInstanceExtensions;
    std::set<std::string> optionalInstanceExtensions; // duplicate entries are allowed between required and optional

    // INSERT LAYERS AND EXTENSIONS HERE


    // Insert extensions required by the windowing system
    // These extensions are platform specific, so SDL will assist with this
    uint32_t displayExtensionCount;
    const char* const *displayExtensions = SDL_Vulkan_GetInstanceExtensions(&displayExtensionCount);
    if(displayExtensions == nullptr) throw std::runtime_error("Failed to get SDL Vulkan instance extensions!");

    for(int ext = 0; ext < displayExtensionCount; ext++)
        requiredInstanceExtensions.insert(displayExtensions[ext]);

    // Insert validation layers and extensions for debug builds only
    #ifndef NDEBUG
        optionalLayers.insert("VK_LAYER_KHRONOS_validation");
        optionalInstanceExtensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    // Compare optional and required layers against the available ones
    // If a required layer is not present, throw a runtime error
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(auto& layer : availableLayers) {
        if(requiredLayers.count(layer.layerName) == 1) {
            requiredLayers.erase(layer.layerName);
            pInstance->enabledInstanceLayers.push_back(layer.layerName);

            continue;
        } else if(optionalLayers.count(layer.layerName) == 1) {
            pInstance->enabledInstanceLayers.push_back(layer.layerName);
        }
    }

    if(!requiredLayers.empty()) {
        for(auto& layer : requiredLayers) {
            std::cerr << "Missing required layer: " << layer << std::endl;
        }
        throw std::runtime_error("Missing one or more required layers!");
    }

    // Compare optional and required extensions against the available ones
    // If a required extension is not present, throw a runtime error
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    for(auto& extension : availableExtensions) {
        if(requiredInstanceExtensions.count(extension.extensionName) == 1) {
            requiredInstanceExtensions.erase(extension.extensionName);
            pInstance->enabledInstanceExtensions.push_back(extension.extensionName);

            continue;
        } else if(optionalInstanceExtensions.count(extension.extensionName) == 1) {
            pInstance->enabledInstanceExtensions.push_back(extension.extensionName);
        }
    }
    
    if(!requiredInstanceExtensions.empty()) {
        for(auto& extension : requiredInstanceExtensions) {
            std::cerr << "Missing required extension: " << extension << std::endl;
        }
        throw std::runtime_error("Missing one or more required instance extensions!");
    }

    std::vector<const char*> extensions;
    for(std::string& extension : pInstance->enabledInstanceExtensions) {
        extensions.push_back(extension.c_str());
    }

    std::vector<const char*> layers;
    for(std::string& layer : pInstance->enabledInstanceLayers) {
        layers.push_back(layer.c_str());
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
    createInfo.enabledLayerCount = layers.size();
    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    // For non-release builds when the debug extension is present, enable a debug callback for instance creation and destruction
    if(pInstance->isExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    
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

    if(vkCreateInstance(&createInfo, nullptr, &pInstance->_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void Instance::createDebugMessenger(Instance* pInstance) {
#ifndef NDEBUG
    // If the extension is available
    if(pInstance->isExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = vkmv::debugCallback;

        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(pInstance->_instance, "vkCreateDebugUtilsMessengerEXT"); // alternatively, use Volk to load functions
        if(!vkCreateDebugUtilsMessengerEXT) throw std::runtime_error("Failed to get ProcAddr: vkCreateDebugUtilsMessengerEXT!");

        if(vkCreateDebugUtilsMessengerEXT(pInstance->_instance, &createInfo, nullptr, &pInstance->_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create debug messenger!");
        }
    }
#endif
}

void Instance::destroyDebugMessenger(Instance* pInstance) {
#ifndef NDEBUG
    // If the extension is available
    if(pInstance->isExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {

        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(pInstance->_instance, "vkDestroyDebugUtilsMessengerEXT");
        if(!vkDestroyDebugUtilsMessengerEXT) throw std::runtime_error("Failed to get proc address: vkDestroyDebugUtilsMessengerEXT!");
        
        vkDestroyDebugUtilsMessengerEXT(pInstance->_instance, pInstance->_debugMessenger, nullptr);
    }
#endif
}

} // namespace vkmv