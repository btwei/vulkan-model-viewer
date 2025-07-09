// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_RENDERER_HPP
#define VKMV_RENDERER_HPP

#include <vector>

#include <vulkan/vulkan.h>

namespace vkmv {

class Renderer {
public:
    Renderer();
    ~Renderer();

    void drawFrame();

private:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    std::vector<char*> enabledInstanceExtensions;


    void initRenderer();
    void cleanup();

    void createInstance();
};

} // namespace vkmv

#endif // VK_MV_RENDERER_HPP