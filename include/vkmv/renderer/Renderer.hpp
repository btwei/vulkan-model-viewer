// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_RENDERER_HPP
#define VKMV_RENDERER_HPP

#include <algorithm>
#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>

namespace vkmv {

struct RenderableState {

};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void drawFrame(RenderableState& r);
    void handleEvent(const SDL_Event& e);

private:
    int frameNumber = 0;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    std::vector<char*> enabledInstanceLayers;
    std::vector<char*> enabledInstanceExtensions;


    void initRenderer();
    void cleanup();

    void createInstance();
    void createDebugMessenger();
    void destroyDebugMessenger();
    
};

} // namespace vkmv

#endif // VK_MV_RENDERER_HPP