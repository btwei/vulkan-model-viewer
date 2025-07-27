// Copyright (c) 2025 Benjamin Wei
//
// This file is part of the vulkan-model-viewer project.
// This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

#include <queue>
#include <stdexcept>

#include "vkmv/core/Device.hpp"

namespace vkmv {

void Device::create(Instance* pInstance, DeviceParams params, Device* pDevice) {
    pickPhysicalDevice(pInstance, params, pDevice);
}

void Device::destroy(Device* pDevice) {
    vkDestroyDevice(pDevice->m_vkDevice, nullptr);
}

/**
 * Enumerates physical devices and selects the best one. Sets pDevice->_physicalDevice and pDevice->enabledExtensions.
 * 
 * If no devices meet the required specs, throws a runtime error
 * 
 * Device suitibility is determined by:
 * - Supporting required extensions
 * - Supporting optional extensions
 * - Possessing a queue family with supportsPresentation and graphicsBit
 * - Must have one or more surface format
 * - Prefer discrete gpus 
 * 
 * @note Perhaps I should refactor this into many smaller functions
 */
void Device::pickPhysicalDevice(Instance* pInstance, DeviceParams params, Device* pDevice) {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(pInstance->getInstance(), &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(pInstance->getInstance(), &deviceCount, physicalDevices.data());

    if(deviceCount == 0) throw std::runtime_error("Failed to gind GPUs with Vulkan support!");

    std::set<std::string> requiredDeviceExtensions;
    std::set<std::string> optionalDeviceExtensions;

    requiredDeviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    requiredDeviceExtensions.insert(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    requiredDeviceExtensions.insert(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    struct CandidateData {
        int score;
        VkPhysicalDevice physicalDevice;
        bool qualified = true;
        std::vector<std::string> candidateEnabledExtensions;

        bool operator<(const CandidateData& other) const {
            return score < other.score;
        }
    };

    std::priority_queue<CandidateData> candidates;

    for(auto& device : physicalDevices) {
        CandidateData deviceTraits;

        deviceTraits.physicalDevice = device;

        // GPU must support all required extensions
        // Prefer selecting GPUs with more desired optional extensions
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());\

        std::set<std::string> candidateRequiredExtensions(requiredDeviceExtensions);
        std::set<std::string> candidateOptionalExtensions(optionalDeviceExtensions);

        for(auto& extension : availableExtensions) {
            if(candidateRequiredExtensions.count(extension.extensionName) == 1) {
                candidateRequiredExtensions.erase(extension.extensionName);
                deviceTraits.candidateEnabledExtensions.push_back(extension.extensionName);

                continue;
            } else if(candidateOptionalExtensions.count(extension.extensionName) == 1) {
                deviceTraits.candidateEnabledExtensions.push_back(extension.extensionName);

                deviceTraits.score += 500;
            }
        }

        if(!candidateRequiredExtensions.empty()) {
            deviceTraits.qualified = false;
            continue;
        }

        // GPU must possess a queue family with graphics support and present support
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        bool graphicsFamilyFound = false;
        bool presentFamilyFound = false; 

        uint32_t i = 0;
        for(const auto& queueFamily : queueFamilies){
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamilyFound = true;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, params.presentableSurface, &presentSupport);

            if(presentSupport) presentFamilyFound = true;

            i++;
        }

        if(!graphicsFamilyFound || !presentFamilyFound) {
            deviceTraits.qualified = false;
            continue;
        }

        // GPU must have at least one available format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, params.presentableSurface, &formatCount, nullptr);

        if(formatCount == 0) {
            deviceTraits.qualified = false;
            continue;
        }

        // Prefer discrete GPUs (which tend to have better performance)
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            deviceTraits.score += 1000;
        }

        if(deviceTraits.qualified == true) candidates.push(deviceTraits);
    }

    if(candidates.top().score > 0){
        pDevice->m_vkPhysicalDevice = candidates.top().physicalDevice;
        pDevice->m_enabledDeviceExtensions = candidates.top().candidateEnabledExtensions;
    } else {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void Device::createDevice(Instance* pInstance, DeviceParams params, Device* pDevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->m_vkPhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->m_vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

    bool graphicsFamilyFound = false;
    bool presentFamilyFound = false;

    // Find the queueFamilies for graphics and present support
    // It's likely that queue 0 will have support for both
    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if(!graphicsFamilyFound && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            pDevice->m_graphicsFamilyIndex = i;
            graphicsFamilyFound = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pDevice->m_vkPhysicalDevice, i, params.presentableSurface, &presentSupport);

        if(!presentFamilyFound && presentSupport) {
            pDevice->m_presentFamilyIndex = i;
            presentFamilyFound = true;
        }

        i++;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { pDevice->m_graphicsFamilyIndex, pDevice->m_presentFamilyIndex };

    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> extensions;
    for(auto& string : pDevice->m_enabledDeviceExtensions) {
        extensions.push_back(string.c_str());
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    VkPhysicalDeviceSynchronization2Features synchronization2{};
    synchronization2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
    dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRendering.dynamicRendering = VK_TRUE;
    dynamicRendering.pNext = &synchronization2;

    createInfo.pNext = &dynamicRendering;

    if(vkCreateDevice(pDevice->m_vkPhysicalDevice, &createInfo, nullptr, &pDevice->m_vkDevice) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create device!");
    }

    vkGetDeviceQueue(pDevice->m_vkDevice, pDevice->m_graphicsFamilyIndex, 0, &pDevice->m_graphicsQueue);
    vkGetDeviceQueue(pDevice->m_vkDevice, pDevice->m_presentFamilyIndex, 0, &pDevice->m_presentQueue);
}

} // namespace vkmv
