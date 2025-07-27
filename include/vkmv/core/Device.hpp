// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_DEVICE_HPP
#define VKMV_DEVICE_HPP

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkmv/core/Instance.hpp"

namespace vkmv {

/**
 * @brief Parameters required to select a physical device and create a logical device.
 */
struct DeviceParams {
    VkSurfaceKHR presentableSurface = VK_NULL_HANDLE;
};

/**
 * @class Device
 * @brief Encapusulates a VkPhysicalDevice and VkDevice.
 * 
 * On initialization, selects the best physical device for vkmv and creates a logical device.
 */
class Device {
public:
    static void create(Instance* pInstance, DeviceParams params, Device* pDevice);

    static void destroy(Device* pDevice);

    VkDevice getDevice() const { return m_vkDevice; }

    VkPhysicalDevice getPhysicalDevice() const { return m_vkPhysicalDevice; }

    uint32_t getGraphicsFamilyIndex() const { return m_graphicsFamilyIndex; }

    uint32_t getPresentFamilyIndex() const { return m_presentFamilyIndex; }

    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }

    VkQueue getPresentQueue() const { return m_presentQueue; }

    bool isExtensionEnabled(const char* extension) const;

private:
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_vkDevice = VK_NULL_HANDLE;

    uint32_t m_graphicsFamilyIndex = 0;
    uint32_t m_presentFamilyIndex = 0;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    std::vector<std::string> m_enabledDeviceExtensions;

    static void pickPhysicalDevice(Instance* pInstance, DeviceParams params, Device* pDevice);
    static void createDevice(Instance* pInstance, DeviceParams params, Device* pDevice);

};

} // namespace vkmv


#endif // VKMV_DEVICE_HPP