// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_RENDERER_HPP
#define VKMV_RENDERER_HPP

#include <functional>
#include <vector>

#include <vulkan/vulkan.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>

#include "vkmv/app/Window.hpp"
#include "vkmv/renderer/ResourceManager.hpp"

namespace vkmv {

constexpr unsigned int NUM_FRAMES_IN_FLIGHT = 2;

struct RenderableState {

};

/**
 * @class Renderer
 * @brief Encapsulates all Vulkan rendering logic.
 * 
 * Intended to be run on a window and receive updates from an engine class.
 */
class Renderer {
public:
    Renderer(const Window& window);
    ~Renderer();

    /**
     * @brief Handles a SDL event. Stores whether the event should be captured by the UIOverlay or not.
     */
    void handleEvent(const SDL_Event& e);

    void drawFrame(RenderableState& r);

private:
    const Window& window;

    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        VkSemaphore swapchainSemaphore;
        VkFence renderFence;

        AllocatedImage renderTargetImage;
    };
    FrameData frames[NUM_FRAMES_IN_FLIGHT];
    int frameCount = 0;
    unsigned int width, height;

    struct swapchainImageResource {
        VkSemaphore renderSemaphore;
    };
    std::vector<swapchainImageResource> swapchainImageResources;

    std::vector<char*> enabledInstanceLayers;
    std::vector<std::string> enabledInstanceExtensions;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
    std::vector<std::string> enabledDeviceExtensions;
    uint32_t graphicsFamilyIndex;
    uint32_t presentFamilyIndex;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    ResourceManager resourceManager;

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
    void createCommandPools();
    void createSyncObjects();
    void createRenderTargets();
    void destroyRenderTargets();

    void initImGUI();
    void cleanupImGUI();

    FrameData& getCurrentFrame();
    void refreshWindowDims();
    void recordMainCommands(RenderableState& r, VkCommandBuffer& buf, VkImage& swapchainImage);
};

} // namespace vkmv

#endif // VK_MV_RENDERER_HPP