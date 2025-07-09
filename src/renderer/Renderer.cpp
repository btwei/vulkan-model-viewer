// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <set>
#include <stdexcept>

#include "vkmv/renderer/Renderer.hpp"

namespace vkmv {

Renderer::Renderer() {

}

Renderer::~Renderer() {

}

void Renderer::drawFrame() {

}

void Renderer::initRenderer() {
    createInstance();
}

void Renderer::cleanup() {

}

void Renderer::createInstance() {
/*
    std::set<char*> requiredLayers;
    std::set<char*> optionalLayers;

    std::set<char*> requiredInstanceExtensions;
    std::set<char*> optionalInstanceExtensions;


    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifndef NDEBUG

    optionalLayers.insert("");

#endif

    VkApplicationInfo appInfo{};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = ;
    createInfo.ppEnabledLayerNames = ;
    createInfo.enabledExtensionCount = ;
    createInfo.ppEnabledExtensionNames = ;

    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
*/
}

} // namespace vkmv