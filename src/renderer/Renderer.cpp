// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <set>
#include <stdexcept>

#include "vkmv/renderer/Renderer.hpp"

namespace vkmv {

Renderer::Renderer() {
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
}

void Renderer::cleanup() {
    vkDestroyInstance(instance, nullptr);
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
    
    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}



} // namespace vkmv