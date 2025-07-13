// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_RENDERER_HPP
#define VKMV_RENDERER_HPP

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <vector>

#include <vulkan/vulkan.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>

#include "vkmv/app/Window.hpp"

namespace vkmv {

struct RenderableState {

};

class Renderer {
public:
    Renderer(const Window& window);
    ~Renderer();

    void drawFrame(RenderableState& r);
    void handleEvent(const SDL_Event& e);

private:
    const Window& window;

    int frameNumber = 0;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t graphicsFamilyIndex;
    uint32_t presentFamilyIndex;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    std::vector<char*> enabledInstanceLayers;
    std::vector<std::string> enabledInstanceExtensions;
    std::vector<std::string> enabledDeviceExtensions;

    void initRenderer();
    void cleanup();

    void createInstance();
    void createDebugMessenger();
    void destroyDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createDevice();
    void createSwapchain();
    void destroySwapchain();

};

} // namespace vkmv

#endif // VK_MV_RENDERER_HPP