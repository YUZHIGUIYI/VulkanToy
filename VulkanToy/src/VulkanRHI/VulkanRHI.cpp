//
// Created by ZHIKANG on 2023/3/27.
//

#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    // In namespace VulkanRHI - variable definition
    size_t VulkanRHI::MaxSwapChainCount = ~0;
    std::atomic<size_t> VulkanRHI::GPUResourceMemoryUsed{0};
    VulkanRHI::DisplayMode VulkanRHI::eDisplayMode = VulkanRHI::DisplayMode::DISPLAYMODE_SDR;
    bool VulkanRHI::isSupportHDR = false;

    VkPhysicalDevice VulkanRHI::GPU     = VK_NULL_HANDLE;
    VkDevice         VulkanRHI::Device  = VK_NULL_HANDLE;
    VmaAllocator     VulkanRHI::VMA     = VK_NULL_HANDLE;
    VkHdrMetadataEXT VulkanRHI::HdrMetadataEXT{ .sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT, .pNext = nullptr };

    ShaderCache* VulkanRHI::ShaderManager = nullptr;
    SamplerCache* VulkanRHI::SamplerManager = nullptr;

    // Push descriptors.
    PFN_vkCmdPushDescriptorSetKHR VulkanRHI::PushDescriptorSetKHR = nullptr;
    PFN_vkCmdPushDescriptorSetWithTemplateKHR VulkanRHI::PushDescriptorSetWithTemplateKHR = nullptr;

    // HDR function
    PFN_vkSetHdrMetadataEXT VulkanRHI::SetHdrMetadataEXT = nullptr;

    static PFN_vkSetDebugUtilsObjectNameEXT GVkSetDebugUtilsObjectName = nullptr;
    static PFN_vkCmdBeginDebugUtilsLabelEXT GVkCmdBeginDebugUtilsLabel = nullptr;
    static PFN_vkCmdEndDebugUtilsLabelEXT   GVkCmdEndDebugUtilsLabel = nullptr;

    void extDebugUtilsGetProcAddresses(VkDevice device)
    {
        GVkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
        GVkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
        GVkCmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
    }

    void additionalGetProcAddresses(VkDevice device)
    {
        VulkanRHI::PushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");
        VulkanRHI::PushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetWithTemplateKHR");
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT type,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char *layerPrefix,
        const char *message,
        void *userData)
    {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            VT_CORE_ERROR("{0}: {1}", layerPrefix, message);
        } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            VT_CORE_WARN("{0}: {1}", layerPrefix, message);
        } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            VT_CORE_INFO("{0}: {1}", layerPrefix, message);
        } else
        {
            VT_CORE_TRACE("{0}: {1}", layerPrefix, message);
        }
        return VK_FALSE;
    }

    void destroyDebugReportCallbackEXT(
        VkInstance instance,
        VkDebugReportCallbackEXT DebugReporter,
        const VkAllocationCallbacks *pAllocator)
    {
        const char* funcName = "vkDestroyDebugReportCallbackEXT";
        auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, funcName);
        if (func != nullptr)
        {
            func(instance, DebugReporter, pAllocator);
        }
        else
        {
            VT_CORE_ERROR("No vulkan function: {} find, maybe exist some driver problem on the machine", funcName);
        }
    }

    VkResult createDebugReportCallbackEXT(
        VkInstance instance,
        const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugReportCallbackEXT *pDebugReporter)
    {
        const char* funcName = "vkCreateDebugReportCallbackEXT";
        auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, funcName);
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugReporter);
        }
        else
        {
            VT_CORE_ERROR("No vulkan function: {} find, maybe exist some driver problem on the machine", funcName);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    bool queryLayersAvailable(const std::vector<const char *> &requiredLayers, const std::vector<VkLayerProperties> &availableLayers)
    {
        if (requiredLayers.empty()) return true;

        for (auto& layer : requiredLayers)
        {
            bool found = false;
            for (auto& available_layer : availableLayers)
            {
                if (std::strcmp(available_layer.layerName, layer) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                VT_CORE_ERROR("No validation layer found: '{0}'", layer);
                return false;
            }
        }
        return true;
    }

    std::vector<const char *> getOptimalValidationLayers(const std::vector<VkLayerProperties> &supportedInstanceLayers)
    {
        // KHRONOS recommends validation layer select priorities.
        std::vector<std::vector<const char*>> validationLayerPriorityList{
            {"VK_LAYER_KHRONOS_validation"},
            {"VK_LAYER_LUNARG_standard_validation"},
            {
                    "VK_LAYER_GOOGLE_threading",
                    "VK_LAYER_LUNARG_parameter_validation",
                    "VK_LAYER_LUNARG_object_tracker",
                    "VK_LAYER_LUNARG_core_validation",
                    "VK_LAYER_GOOGLE_unique_objects",
            },
            {"VK_LAYER_LUNARG_core_validation"}
        };
        for (const auto& validationLayers : validationLayerPriorityList)
        {
            if (queryLayersAvailable(validationLayers, supportedInstanceLayers))
            {
                return validationLayers;
            }
            VT_CORE_WARN("Can not open validation layer! Vulkan run without debug layer");
        }
        return {};
    }

    VkFormat VulkanContext::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
    {
        return m_device.findSupportedFormat(candidates, tiling, featureFlags);
    }

    int32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags)
    {
        return m_device.findMemoryType(typeFilter, memoryPropertyFlags);
    }

    void VulkanContext::PresentContext::init()
    {
        VT_CORE_ASSERT(VulkanRHI::MaxSwapChainCount != ~0, "Vulkan RHI GMaxSwpChainCount must be initialized");

        semaphoresImageAvailable.resize(VulkanRHI::MaxSwapChainCount);
        semaphoresRenderFinished.resize(VulkanRHI::MaxSwapChainCount);

        inFlightFences.resize(VulkanRHI::MaxSwapChainCount);
        imagesInFlight.resize(VulkanRHI::MaxSwapChainCount);
        for (auto& fence : imagesInFlight)
        {
            fence = VK_NULL_HANDLE;
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < VulkanRHI::MaxSwapChainCount; ++i)
        {
            RHICheck(vkCreateSemaphore(VulkanRHI::Device, &semaphoreCreateInfo, nullptr, &semaphoresImageAvailable[i]));
            RHICheck(vkCreateSemaphore(VulkanRHI::Device, &semaphoreCreateInfo, nullptr, &semaphoresRenderFinished[i]));
            RHICheck(vkCreateFence(VulkanRHI::Device, &fenceCreateInfo, nullptr, &inFlightFences[i]));
        }
    }

    void VulkanContext::PresentContext::release()
    {
        for (size_t i = 0; i < VulkanRHI::MaxSwapChainCount; ++i)
        {
            vkDestroySemaphore(VulkanRHI::Device, semaphoresImageAvailable[i], nullptr);
            vkDestroySemaphore(VulkanRHI::Device, semaphoresRenderFinished[i], nullptr);
            vkDestroyFence(VulkanRHI::Device, inFlightFences[i], nullptr);
        }
    }

    void VulkanContext::initInstance(const std::vector<const char *> &requiredExtensions, const std::vector<const char *> &requiredLayers)
    {
        bool isUtilDebug = false;

        std::vector<const char *> enableExtensions{};

        // Query all useful instance extensions
        uint32_t instanceExtensionCount;
        RHICheck(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));
        std::vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
        RHICheck(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));

        // Prepare debug extensions
        for (const auto& availableExtension : availableInstanceExtensions)
        {
            if (std::strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                VT_CORE_INFO("Extension '{0}' is enabled", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                enableExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }
        if (!isUtilDebug)
        {
            // When debug util unused, open debug report extension
            enableExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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
        std::vector<const char *> requestedInstanceLayers(requiredLayers);
        if (!queryLayersAvailable(requestedInstanceLayers, supportedInstanceLayers))
        {
            VT_CORE_CRITICAL("No instance layer found");
        }

        // Validation layer
        auto validationLayers = getOptimalValidationLayers(supportedInstanceLayers);
        requestedInstanceLayers.insert(requestedInstanceLayers.end(), validationLayers.begin(), validationLayers.end());
        for (const auto& layer : requestedInstanceLayers)
        {
            VT_CORE_INFO("Requested layers enabled: '{0}'", layer);
        }

        // Vulkan info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VulkanRHI";
        appInfo.applicationVersion = 0;
        appInfo.pEngineName = "VulkanToy";
        appInfo.engineVersion = 0;
        appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);

        // Instance info
        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = enableExtensions.data();
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requestedInstanceLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = requestedInstanceLayers.data();

        VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo{};
        debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        debugReportCreateInfo.pfnCallback = debugReportCallback;
        instanceCreateInfo.pNext = &debugReportCreateInfo;

        // Create vulkan instance
        RHICheck(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));
        RHICheck(createDebugReportCallbackEXT(m_instance, &debugReportCreateInfo, nullptr, &m_debugReportHandle));
    }

    void VulkanContext::releaseInstance()
    {
        if (m_debugReportHandle != VK_NULL_HANDLE)
        {
            destroyDebugReportCallbackEXT(m_instance, m_debugReportHandle, nullptr);
            m_debugReportHandle = VK_NULL_HANDLE;
        }
        if (m_instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }

    void VulkanContext::initDevice(VkPhysicalDeviceFeatures features, const std::vector<const char *> &requestedExtensions, void *nextChain)
    {
        m_device.init(features, requestedExtensions, nextChain);
    }

    void VulkanContext::releaseDevice()
    {
        m_device.release();
    }

    void VulkanContext::initVMA()
    {
        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.physicalDevice = m_device.physicalDevice;
        allocatorCreateInfo.device = m_device.logicalDevice;
        allocatorCreateInfo.instance = m_instance;
        //allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;  // Support Ray Tracing
        vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator);
    }

    void VulkanContext::releaseVMA()
    {
        vmaDestroyAllocator(m_vmaAllocator);
    }

    void VulkanContext::createDrawCommandBuffers()
    {
        m_drawCmdBuffers.resize(m_swapChain.imageCount);

        VkCommandBufferAllocateInfo cmdAllocateInfo{};
        cmdAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_drawCmdBuffers.size());
        cmdAllocateInfo.commandPool = m_device.majorGraphicsPool.pool;
        cmdAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        RHICheck(vkAllocateCommandBuffers(m_device.logicalDevice, &cmdAllocateInfo, m_drawCmdBuffers.data()));
    }

    void VulkanContext::init(GLFWwindow *window)
    {
        m_window = window;

        // Initialize instance
        {
            std::vector<const char *> instanceExtensionNames;
            instanceExtensionNames.reserve(4);
            instanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            instanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            instanceExtensionNames.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME); // TODO

            std::vector<const char *> instanceLayerNames{};
            initInstance(instanceExtensionNames, instanceLayerNames);
        }

        // Initialize surface
        if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS)
        {
            VT_CORE_CRITICAL("Fail to create window surface");
        }

        // Initialize device
        {
            std::vector<const char *> deviceExtensionNames;
            deviceExtensionNames.reserve(10);
            deviceExtensionNames.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            deviceExtensionNames.push_back(VK_EXT_HDR_METADATA_EXTENSION_NAME); // TODO

            // TODO: RTX ON

            // Current only nvidia support mesh shader, so we don't use it, we simulate by compute shader.
            VkPhysicalDeviceFeatures         enable10GpuFeatures{}; // vulkan 1.0
            VkPhysicalDeviceVulkan11Features enable11GpuFeatures{}; // vulkan 1.1
            VkPhysicalDeviceVulkan12Features enable12GpuFeatures{}; // vulkan 1.2
            VkPhysicalDeviceVulkan13Features enable13GpuFeatures{}; // vulkan 1.3

            // Enable gpu features 1.0 here.
            enable10GpuFeatures.samplerAnisotropy = true;
            enable10GpuFeatures.depthClamp = true;
            enable10GpuFeatures.shaderSampledImageArrayDynamicIndexing = true;
            enable10GpuFeatures.multiDrawIndirect = VK_TRUE;
            enable10GpuFeatures.drawIndirectFirstInstance = VK_TRUE;
            enable10GpuFeatures.independentBlend = VK_TRUE;
            enable10GpuFeatures.multiViewport = VK_TRUE;
            enable10GpuFeatures.fragmentStoresAndAtomics = VK_TRUE;
            enable10GpuFeatures.shaderInt16 = VK_TRUE;

            // Enable gpu features 1.1 here.
            enable11GpuFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            enable11GpuFeatures.pNext = &enable12GpuFeatures;
            enable11GpuFeatures.shaderDrawParameters = VK_TRUE;

            // Enable gpu features 1.2 here.
            enable12GpuFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            enable12GpuFeatures.pNext = &enable13GpuFeatures;
            enable12GpuFeatures.drawIndirectCount = VK_TRUE;
            enable12GpuFeatures.drawIndirectCount = VK_TRUE;
            enable12GpuFeatures.imagelessFramebuffer = VK_TRUE;
            enable12GpuFeatures.separateDepthStencilLayouts = VK_TRUE;
            enable12GpuFeatures.descriptorIndexing = VK_TRUE;
            enable12GpuFeatures.runtimeDescriptorArray = VK_TRUE;
            enable12GpuFeatures.descriptorBindingPartiallyBound = VK_TRUE;
            enable12GpuFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
            enable12GpuFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            enable12GpuFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
            enable12GpuFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
            enable12GpuFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
            enable12GpuFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
            enable12GpuFeatures.timelineSemaphore = VK_TRUE;
            enable12GpuFeatures.bufferDeviceAddress = VK_TRUE;

            // Enable gpu features 1.3 here.
            enable13GpuFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            enable13GpuFeatures.dynamicRendering = VK_TRUE;
            enable13GpuFeatures.synchronization2 = VK_TRUE;
            enable13GpuFeatures.maintenance4 = VK_TRUE;

            VkPhysicalDeviceAccelerationStructureFeaturesKHR enableASFeatures{};
            enableASFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            enableASFeatures.accelerationStructure = VK_TRUE;
            enable13GpuFeatures.pNext = &enableASFeatures;

            VkPhysicalDeviceRayTracingPipelineFeaturesKHR enableRTPipelineFeatures{};
            enableRTPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            enableRTPipelineFeatures.rayTracingPipeline = VK_TRUE;
            enableASFeatures.pNext = &enableRTPipelineFeatures;

            initDevice(enable10GpuFeatures, deviceExtensionNames, &enable11GpuFeatures);
        }

        VulkanRHI::GPU = m_device.physicalDevice;
        VulkanRHI::Device = m_device.logicalDevice;

        // Find some proc address
        extDebugUtilsGetProcAddresses(m_device.logicalDevice);
        additionalGetProcAddresses(m_device.logicalDevice);

        // Initialize VMA
        initVMA();
        VulkanRHI::VMA = m_vmaAllocator;

        // Initialize shader cache
        m_shaderCache.init();
        VulkanRHI::ShaderManager = &m_shaderCache;

        // Initialize sampler cache
        m_samplerCache.init();
        VulkanRHI::SamplerManager = &m_samplerCache;

        // Initialize descriptor cache(layout and descriptor set)
        m_descriptorPoolCache.init();

        // Initialize swap chain
        m_swapChain.init();
        VulkanRHI::MaxSwapChainCount = m_swapChain.swapChainImageViews.size();

        // Initialize present context
        m_presentContext.init();

        // Initialize command pool
        m_device.initCommandPool();

        // Create draw frame command buffers
        createDrawCommandBuffers();

        // TODO: other

    }

    void VulkanContext::release()
    {
        // TODO: other

        // Release present context
        m_presentContext.release();

        // Release swap chain
        m_swapChain.release();

        // TODO: release descriptor cache(layout and descriptor set)
        m_descriptorPoolCache.release();

        // TODO: release sampler cache
        m_samplerCache.release();

        // Release VMA
        releaseVMA();

        // Release device and its command pools
        releaseDevice();

        // Release surface
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

        // Release instance
        releaseInstance();
    }

    bool VulkanContext::isSwapChainRebuilt()
    {
        glfwGetWindowSize(m_window, &currentWidth, &currentHeight);

        if (currentWidth != lastWidth || currentHeight != lastHeight)
        {
            lastWidth = currentWidth;
            lastHeight = currentHeight;
            return true;
        }
        return false;
    }

    void VulkanContext::rebuildSwapChain()
    {
        vkDeviceWaitIdle(m_device.logicalDevice);

        static int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);

        if (width == 0 || height == 0)
        {
            m_presentContext.isSwapChainChange = true;
            return;
        }

        // TODO: before recreate broadcast

        // Clean
        m_presentContext.release();
        m_swapChain.rebuild();
        m_presentContext.init();
        m_presentContext.imagesInFlight.resize(m_swapChain.swapChainImageViews.size(), VK_NULL_HANDLE);

        // TODO: after recreate broadcast
        onAfterSwapChainRebuild.broadcast();
    }

    SwapChainSupportDetails VulkanContext::querySwapChainSupportDetail()
    {
        SwapChainSupportDetails details{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.physicalDevice, m_surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.physicalDevice, m_surface, &formatCount, nullptr);
        if (formatCount > 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.physicalDevice, m_surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.physicalDevice, m_surface, &presentModeCount, nullptr);
        if (presentModeCount > 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.physicalDevice, m_surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    uint32_t VulkanContext::acquireNextPresentImage()
    {
        m_presentContext.isSwapChainChange |= isSwapChainRebuilt();

        vkWaitForFences(m_device.logicalDevice, 1, &m_presentContext.inFlightFences[m_presentContext.currentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(
            m_device.logicalDevice,
            m_swapChain.swapChain,
            UINT64_MAX,
            m_presentContext.semaphoresImageAvailable[m_presentContext.currentFrame],
            VK_NULL_HANDLE,
            &m_presentContext.imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            rebuildSwapChain();
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            VT_CORE_CRITICAL("Fail to acquire presented image");
        }

        if (m_presentContext.imagesInFlight[m_presentContext.imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_device.logicalDevice, 1, &m_presentContext.imagesInFlight[m_presentContext.imageIndex], VK_TRUE, UINT64_MAX);
        }

        m_presentContext.imagesInFlight[m_presentContext.imageIndex] = m_presentContext.inFlightFences[m_presentContext.currentFrame];

        return m_presentContext.imageIndex;
    }

    void VulkanContext::present()
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signalSemaphores[] = { m_presentContext.semaphoresRenderFinished[m_presentContext.currentFrame] };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { m_swapChain.swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &m_presentContext.imageIndex;

        VkResult result = vkQueuePresentKHR(m_device.majorGraphicsPool.queue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_presentContext.isSwapChainChange)
        {
            m_presentContext.isSwapChainChange = false;
            rebuildSwapChain();
        } else if (result != VK_SUCCESS)
        {
            VT_CORE_CRITICAL("Fail to present image");
        }
        // If swap chain rebuilt and on minimized, still add frame
        m_presentContext.currentFrame = (m_presentContext.currentFrame + 1) % VulkanRHI::MaxSwapChainCount;
    }

    void VulkanContext::submit(uint32_t count, VkSubmitInfo *infos)
    {
        RHICheck(vkQueueSubmit(m_device.majorGraphicsPool.queue, count, infos, m_presentContext.inFlightFences[m_presentContext.currentFrame]));
    }

    void VulkanContext::submitWithoutFence(uint32_t count, VkSubmitInfo *infos)
    {
        RHICheck(vkQueueSubmit(m_device.majorGraphicsPool.queue, count, infos, nullptr));
    }

    void VulkanContext::resetFence()
    {
        RHICheck(vkResetFences(m_device.logicalDevice, 1, &m_presentContext.inFlightFences[m_presentContext.currentFrame]));
    }

    VkCommandBuffer VulkanContext::createMajorGraphicsCommandBuffer()
    {
        VkCommandBufferAllocateInfo cmdAllocateInfo{};
        cmdAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocateInfo.commandBufferCount = 1;
        cmdAllocateInfo.commandPool = m_device.majorGraphicsPool.pool;
        cmdAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer cmdBuffer;
        RHICheck(vkAllocateCommandBuffers(m_device.logicalDevice, &cmdAllocateInfo, &cmdBuffer));

        return cmdBuffer;
    }

    DescriptorFactory VulkanContext::descriptorFactoryBegin()
    {
        return DescriptorFactory::begin(&m_descriptorPoolCache);
    }

    VkPipelineLayout VulkanContext::createPipelineLayout(const VkPipelineLayoutCreateInfo &info)
    {
        VkPipelineLayout pipelineLayout;
        RHICheck(vkCreatePipelineLayout(m_device.logicalDevice, &info, nullptr, &pipelineLayout));
        return pipelineLayout;
    }

    // In namespace VulkanRHI - functions
    void VulkanRHI::setResourceName(VkObjectType objectType, uint64_t handle, const char *name)
    {
        // TODO: check
        static std::mutex gMutexForSettingResource;
        if (GVkSetDebugUtilsObjectName && handle && name)
        {
            std::unique_lock<std::mutex> lockGuard{gMutexForSettingResource};
            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = handle;
            nameInfo.pObjectName = name;
            GVkSetDebugUtilsObjectName(VulkanRHI::Device, &nameInfo);
        }
    }

    void VulkanRHI::setPerfMarkerBegin(VkCommandBuffer cmdBuf, const char *name, const glm::vec4 &color)
    {
        if (GVkCmdBeginDebugUtilsLabel)
        {
            VkDebugUtilsLabelEXT labelExt{};
            labelExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            labelExt.pLabelName = name;
            labelExt.color[0] = color.r;
            labelExt.color[1] = color.g;
            labelExt.color[2] = color.b;
            labelExt.color[3] = color.a;
            GVkCmdBeginDebugUtilsLabel(cmdBuf, &labelExt);
        }
    }

    void VulkanRHI::setPerfMarkerEnd(VkCommandBuffer cmdBuf)
    {
        if (GVkCmdEndDebugUtilsLabel)
        {
            GVkCmdEndDebugUtilsLabel(cmdBuf);
        }
    }

    void VulkanRHI::executeImmediately(VkCommandPool commandPool, VkQueue queue, std::function<void(VkCommandBuffer)> &&func)
    {
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = commandPool;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer{};
        vkAllocateCommandBuffers(VulkanRHI::Device, &allocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        func(commandBuffer);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(VulkanRHI::Device, commandPool, 1, &commandBuffer);
    }

    void VulkanRHI::executeImmediatelyMajorGraphics(std::function<void(VkCommandBuffer)> &&func)
    {
        executeImmediately(VulkanRHI::get()->getMajorGraphicsCommandPool(), VulkanRHI::get()->getMajorGraphicsQueue(), std::move(func));
    }

    void VulkanRHI::addGPUResourceMemoryUsed(size_t in)
    {
        GPUResourceMemoryUsed.fetch_add(in);
    }

    void VulkanRHI::minusGPUResourceMemoryUsed(size_t in)
    {
        GPUResourceMemoryUsed.fetch_sub(in);
    }

    size_t VulkanRHI::getGPUResourceMemoryUsed()
    {
        return GPUResourceMemoryUsed.load();
    }
}




















