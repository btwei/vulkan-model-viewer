// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_INSTANCE_HPP
#define VKMV_INSTANCE_HPP

#include <array>
#include <set>
#include <vector>

#include <vulkan/vulkan.h>

namespace vkmv {

/**
 * @class Instance
 * @brief Encapsulates a Vulkan Instance.
 * 
 * Initializes an Instance object with a VkInstance. If NDEBUG is not defined, validation layers will
 * be enabled automatically (in other words, validation layers are enabled for DEBUG builds, not RELEASE
 * builds). Platform specific window extensions are added automatically.
 * 
 * @note I am trying out create/destroy methods instead of using RAII this time to see how I like it.
 */
class Instance {
public:
    /**
     * @brief Initializes an Instance object with a VkInstance and optionally a VkDebugUtilsMessenger.
     * 
     * Not a library function. Setup is specific to vkmv.
     */
    static void create(Instance* pInstance);

    /**
     * @brief Destroys an Instance object. Do not use after destroying.
     */
    static void destroy(Instance* pInstance);

    VkInstance getInstance() const { return _instance; }

    VkDebugUtilsMessengerEXT getDebugMessenger() const { return _debugMessenger; }

    /**
     * @brief Returns true if given instance extension is enabled.
     */
    bool isExtensionEnabled(const char* extension) const;

private:
    static void createInstance(Instance* pInstance);
    static void createDebugMessenger(Instance* pInstance);
    static void destroyDebugMessenger(Instance* pInstance);

    VkInstance _instance = VK_NULL_HANDLE;
    std::vector<std::string> enabledInstanceLayers;
    std::vector<std::string> enabledInstanceExtensions;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
};

} // namespace vkmv

#endif // VKMV_INSTANCE_HPP