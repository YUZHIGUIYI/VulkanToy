//
// Created by ZHIKANG on 2023/3/27.
//

#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    bool checkDeviceExtensionSupport(const std::vector<const char *>& requestDeviceExtensions, VkPhysicalDevice physicalDevice)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(requestDeviceExtensions.begin(), requestDeviceExtensions.end());
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    bool VulkanContext::isPhysicalDeviceSuitable(const std::vector<const char *> &requestExtensions)
    {
        bool isAllQueueExist = false;
        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(m_device.physicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(m_device.physicalDevice, &queueFamilyCount, queueFamilies.data());

            bool isSupportGraphics = false;
            bool isSupportCompute = false;
            bool isSupportCopy = false;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    isSupportGraphics = true;
                }
                if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    isSupportCompute = true;
                }
                if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    isSupportCopy = true;
                }
            }
            isAllQueueExist = isSupportGraphics && isSupportCompute && isSupportCopy;
        }

        bool isExtensionsSupport = checkDeviceExtensionSupport(requestExtensions, m_device.physicalDevice);

        bool isSwapChainAdequate = true;
        if (isExtensionsSupport)
        {
            // Add
        }

        return isAllQueueExist && isExtensionsSupport && isSwapChainAdequate;
    }

    void VulkanContext::pickupSuitableGPU(const std::vector<const char *> &requestExtensions)
    {
        uint32_t physicalDeviceCount;
        RHICheck(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr));
        if (physicalDeviceCount < 1)
        {
            VT_CORE_CRITICAL("No gpu support vulkan api on this computer");
        }
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        RHICheck(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data()));
        for (const auto& GPU : physicalDevices)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(GPU, &deviceProperties);
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                m_device.physicalDevice = GPU;
                if (isPhysicalDeviceSuitable(requestExtensions))
                {
                    VT_CORE_INFO("Using discrete GPU: {0}", deviceProperties.deviceName);
                    m_device.properties = deviceProperties;
                    return;
                }
            }
        }

        VT_CORE_WARN("No discrete GPU found, using default one");

        for (const auto& GPU : physicalDevices)
        {
            m_device.physicalDevice = GPU;
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(GPU, &deviceProperties);
            if (isPhysicalDeviceSuitable(requestExtensions))
            {
                VT_CORE_INFO("Using discrete GPU: {0}", deviceProperties.deviceName);
                m_device.properties = deviceProperties;
                return;
            }
        }
        VT_CORE_CRITICAL("No suitable GPU found");
    }

    void VulkanContext::initInstance(const std::vector<const char *> &requiredExtensions, const std::vector<const char *> &requiredLayers)
    {
        std::vector<const char *> enableExtensions{};

        // Query all useful instance extensions
        uint32_t instanceExtensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data());

        // Prepare debug extensions
        for (const auto& availableExtension : availableInstanceExtensions)
        {
            if (std::strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                VT_CORE_INFO("Extension '{0}' is enabled", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                enableExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }

        // Surface extensions
        enableExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        // GLFW extensions
        {
            uint32_t glfwExtensionCount;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
            enableExtensions.insert(enableExtensions.end(), extensions.begin(), extensions.end());
        }

        // Other extensions
        for (const auto& availableExtension : availableInstanceExtensions)
        {
            if (std::strcmp(availableExtension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
            {
                VT_CORE_INFO("Extension '{0}' is enabled", VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                enableExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }
        }

        // Add input requested instance extensions
        for (const auto& extension : requiredExtensions)
        {
            if (std::find_if(availableInstanceExtensions.begin(), availableInstanceExtensions.end(),
                                [&extension](VkExtensionProperties availableExtension)
                                {
                                    return std::strcmp(availableExtension.extensionName, extension) == 0;
                                }) == availableInstanceExtensions.end())
            {
                VT_CORE_CRITICAL("Extension '{0}' is no useful", extension);
            }
            else
            {
                enableExtensions.push_back(extension);
            }
        }

        // Query all supported instance layer
        uint32_t instanceLayerCount;
        RHICheck(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
        std::vector<VkLayerProperties> supportedInstanceLayers(instanceLayerCount);
        RHICheck(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedInstanceLayers.data()));

        // Instance layer.
        std::vector<const char*> requestedInstanceLayers(requiredLayers);

        // Validation layer


        // Vulkan info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.applicationVersion = 0;
        appInfo.pEngineName = "VulkanToy";
        appInfo.engineVersion = 0;
        appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);

        // Instance info
        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = enableExtensions.data();
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requestedInstanceLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = requestedInstanceLayers.data();
        instanceCreateInfo.pNext = nullptr; // Add

        // Create vulkan instance
        RHICheck(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));




    }
}




















