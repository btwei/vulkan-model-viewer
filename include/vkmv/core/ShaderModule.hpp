// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#ifndef VKMV_SHADERMODULE_HPP
#define VKMV_SHADERMODULE_HPP

#include <vulkan/vulkan.h>

#include "vkmv/core/Device.hpp"

namespace vkmv {

/**
 * @class ShaderModule
 * @brief Encapsulates Vulkan ShaderModules with RAII principles.
 * 
 * 
 */
class ShaderModule {
public:
    static VkResult Create();

    ~ShaderModule();

    ShaderModule(const ShaderModule&) = delete;
    ShaderModule operator=(const ShaderModule&) = delete;

    VkShaderModule getShaderModule() const { return _shaderModule; }

private:
    Device* device;
    VkShaderModule _shaderModule = VK_NULL_HANDLE;

};

} // namespace vkmv

#endif // VKMV_SHADERMODULE_HPP