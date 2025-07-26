// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_DEVICE_HPP
#define VKMV_DEVICE_HPP

#include <vulkan/vulkan.h>

namespace vkmv {

/**
 * @class Device
 * @brief Encapusulates a VkDevice
 * 
 */
class Device {
public:
    static void create(Device* pDevice);

    static void destroy(Device* pDevice);

    VkDevice getDevice() const { return _device; }

private:
    VkDevice _device = VK_NULL_HANDLE;

};

} // namespace vkmv


#endif // VKMV_DEVICE_HPP