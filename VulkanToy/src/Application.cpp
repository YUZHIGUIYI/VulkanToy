//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/Core/Application.h>
#include <VulkanToy/Core/VulkanTools.h>
#include <VulkanToy/Core/Input.h>
#include <GLFW/glfw3.h>

namespace VT
{
    Application* Application::s_Instance = nullptr;

    std::vector<const char*> Application::s_Args;

    Application::Application(const std::string &name, ApplicationCommandLineArgs args, bool enableValidation)
        : m_Name(name), m_CommandLineArgs(args)
    {
        VT_CORE_ASSERT(!s_Instance, "Application already exists!");

        // Create glfw window
        s_Instance = this;
        m_Window = Window::Create(WindowProps{name});
        m_Window->SetEventCallback(VT_BIND_EVENT_FN(Application::OnEvent));

        // External
        m_Settings.validation = enableValidation;

        // Command line arguments
        m_CommandLineParser.add("help", { "--help" }, 0, "Show help");
        m_CommandLineParser.add("validation", { "-v", "--validation" }, 0, "Enable validation layers");
        m_CommandLineParser.add("vsync", { "-vs", "--vsync" }, 0, "Enable V-Sync");
        m_CommandLineParser.add("fullscreen", { "-f", "--fullscreen" }, 0, "Start in fullscreen mode");
        m_CommandLineParser.add("width", { "-w", "--width" }, 1, "Set window width");
        m_CommandLineParser.add("height", { "-h", "--height" }, 1, "Set window height");
        m_CommandLineParser.add("shaders", { "-s", "--shaders" }, 1, "Select shader type to use (glsl or hlsl)");
        m_CommandLineParser.add("gpuselection", { "-g", "--gpu" }, 1, "Select GPU to run on");
        m_CommandLineParser.add("gpulist", { "-gl", "--listgpus" }, 0, "Display a list of available Vulkan devices");
        m_CommandLineParser.add("benchmark", { "-b", "--benchmark" }, 0, "Run example in benchmark mode");
        m_CommandLineParser.add("benchmarkwarmup", { "-bw", "--benchwarmup" }, 1, "Set warmup time for benchmark mode in seconds");
        m_CommandLineParser.add("benchmarkruntime", { "-br", "--benchruntime" }, 1, "Set duration time for benchmark mode in seconds");
        m_CommandLineParser.add("benchmarkresultfile", { "-bf", "--benchfilename" }, 1, "Set file name for benchmark results");
        m_CommandLineParser.add("benchmarkresultframes", { "-bt", "--benchframetimes" }, 0, "Save frame times to benchmark results file");
        m_CommandLineParser.add("benchmarkframes", { "-bfs", "--benchmarkframes" }, 1, "Only render the given number of frames");

        m_CommandLineParser.parse(s_Args);

        if (m_CommandLineParser.isSet("validation")) {
            m_Settings.validation = true;
        }
        if (m_CommandLineParser.isSet("vsync")) {
            m_Settings.vsync = true;
        }
        if (m_CommandLineParser.isSet("fullscreen")) {
            m_Settings.fullscreen = true;
        }
        if (m_CommandLineParser.isSet("shaders")) {
            std::string value = m_CommandLineParser.getValueAsString("shaders", "glsl");
            if ((value != "glsl") && (value != "hlsl")) {
                VT_CORE_WARN("Shader type must be one of 'glsl' or 'hlsl'");
            }
            else {
                shaderDir = value;
            }
        }

        // Vulkan init
        InitVulkan();

        // TODO: Fix ImGui init
        m_ImGuiLayer = new ImGuiLayer();
        //PushOverlay(m_ImGuiLayer);    // Make if after Prepare function()
    }

    Application::~Application()
    {
        // External
        // Clean up Vulkan resources
        m_SwapChain.cleanup();
        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
        }
        DestroyCommandBuffers();
        if (m_RenderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        }
        for (uint32_t i = 0; i < m_FrameBuffers.size(); i++)
        {
            vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
        }

        for (auto& shaderModule : m_ShaderModules)
        {
            vkDestroyShaderModule(m_Device, shaderModule, nullptr);
        }
        vkDestroyImageView(m_Device, m_DepthStencil.view, nullptr);
        vkDestroyImage(m_Device, m_DepthStencil.image, nullptr);
        vkFreeMemory(m_Device, m_DepthStencil.mem, nullptr);

        vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);

        vkDestroyCommandPool(m_Device, m_CmdPool, nullptr);

        vkDestroySemaphore(m_Device, m_Semaphores.presentComplete, nullptr);
        vkDestroySemaphore(m_Device, m_Semaphores.renderComplete, nullptr);
        for (auto& fence : m_WaitFences) {
            vkDestroyFence(m_Device, fence, nullptr);
        }

        delete m_VulkanDevice;

        vkDestroyInstance(m_Instance, nullptr);
    }

    bool Application::InitVulkan()
    {
        // Vulkan instance
        VkResult err = CreateInstance(m_Settings.validation);
        if (err != VK_SUCCESS)
        {
            VT_CORE_ERROR("Fail to create Vulkan instance");
            return false;
        }

        // Physical device
        uint32_t gpuCount = 0;
        // Get number of available physical devices
        vkEnumeratePhysicalDevices(m_Instance, &gpuCount, nullptr);
        if (gpuCount == 0)
        {
            VT_CORE_ERROR("No Device with Vulkan support found");
            return false;
        }
        // Enumerate devices
        std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
        vkEnumeratePhysicalDevices(m_Instance, &gpuCount, physicalDevices.data());

        // GPU selection

        // Select physical device to be used for the Vulkan example
        // Defaults to the first device unless specified by command line
        uint32_t selectedDevice = 0;
        // GPU selection via command line argument
        if (m_CommandLineParser.isSet("gpuselection")) {
            uint32_t index = m_CommandLineParser.getValueAsInt("gpuselection", 0);
            if (index > gpuCount - 1) {
                VT_CORE_INFO("Selected device index {0} is out of range, reverting to device 0 (use -listgpus to show available Vulkan devices)", index);
            } else {
                selectedDevice = index;
            }
        }
        if (m_CommandLineParser.isSet("gpulist")) {
            VT_CORE_INFO("Available Vulkan devices");
            for (uint32_t i = 0; i < gpuCount; i++) {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
                VT_CORE_INFO("Device [{0}] : {1}", i, deviceProperties.deviceName);
                VT_CORE_INFO(" API: {0}.{1}.{2}", (deviceProperties.apiVersion >> 22), ((deviceProperties.apiVersion >> 12) & 0x3ff), (deviceProperties.apiVersion & 0xfff));
            }
        }

        m_PhysicalDevice = physicalDevices[selectedDevice];

        // Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
        vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_DeviceFeatures);
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_DeviceMemoryProperties);

        // Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
        GetEnabledFeatures();

        // Vulkan device creation
        // This is handled by a separate class that gets a logical device representation
        // and encapsulates functions related to a device
        m_VulkanDevice = new VulkanDevice(m_PhysicalDevice);

        // Derived examples can enable extensions based on the list of supported extensions read from the physical device
        GetEnabledExtensions();

        VkResult res = m_VulkanDevice->createLogicalDevice(m_EnabledFeatures, m_EnabledDeviceExtensions, m_DeviceCreatepNextChain);
        if (res != VK_SUCCESS) {
            VT_CORE_ERROR("Fail to create Vulkan device");
            return false;
        }
        m_Device = m_VulkanDevice->logicalDevice;

        // Get a graphics queue from the device
        vkGetDeviceQueue(m_Device, m_VulkanDevice->queueFamilyIndices.graphics, 0, &m_Queue);

        // Find a suitable depth format
        VkBool32 validDepthFormat = TOOLS::getSupportedDepthFormat(m_PhysicalDevice, &m_DepthFormat);
        VT_CORE_ASSERT(validDepthFormat, "Fail to get supported depth format");

        m_SwapChain.connect(m_Instance, m_PhysicalDevice, m_Device, (GLFWwindow*)m_Window->GetNativeWindow());

        // Create synchronization objects
        VkSemaphoreCreateInfo semaphoreCreateInfo = INITIALIZERS::semaphoreCreateInfo();
        // Create a semaphore used to synchronize image presentation
        // Ensures that the image is displayed before we start submitting new commands to the queue
        vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Semaphores.presentComplete);
        // Create a semaphore used to synchronize command submission
        // Ensures that the image is not presented until all commands have been submitted and executed
        vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Semaphores.renderComplete);

        // Set up submit info structure
        // Semaphores will stay the same during application lifetime
        // Command buffer submission info is set by each example
        m_SubmitInfo = INITIALIZERS::submitInfo();
        m_SubmitInfo.pWaitDstStageMask = &m_SubmitPipelineStages;
        m_SubmitInfo.waitSemaphoreCount = 1;
        m_SubmitInfo.pWaitSemaphores = &m_Semaphores.presentComplete;
        m_SubmitInfo.signalSemaphoreCount = 1;
        m_SubmitInfo.pSignalSemaphores = &m_Semaphores.renderComplete;

        // TODO: separate
        // Create Descriptor pool
        VkDescriptorPoolSize poolSizes[] =
            {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * static_cast<uint32_t>((sizeof(poolSizes) / sizeof(*(poolSizes))));
        poolInfo.poolSizeCount = static_cast<uint32_t>((sizeof(poolSizes) / sizeof(*(poolSizes))));
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.pNext = nullptr;
        if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to create descriptor pool");

        return true;
    }

    VkResult Application::CreateInstance(bool enableValidation)
    {
        // External
        this->m_Settings.validation = enableValidation;

        // Validation can also be forced via a define
#if defined(_VALIDATION)
        this->m_Settings.validation = true;
#endif

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = m_Name.c_str();
        appInfo.pEngineName = "No Engine";
        appInfo.apiVersion = m_ApiVersion;

        //std::vector<const char*> instanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME };
        std::vector<const char*> instanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

        // Use GLFW as window api
        uint32_t glfwExtensionsCount;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        for (uint32_t i = 0; i < glfwExtensionsCount; ++i)
        {
            instanceExtensions.push_back(glfwExtensions[i]);
            VT_CORE_INFO("GLFW instance extension: {0}", glfwExtensions[i]);
        }

        // Get extensions supported by the instance and store for later use
        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data()) == VK_SUCCESS)
            {
                for (VkExtensionProperties& extension : extensions)
                {
                    m_SupportedInstanceExtensions.emplace_back(extension.extensionName);
                }
            }
        }

        // Enabled requested instance extensions
        if (!m_EnabledInstanceExtensions.empty())
        {
            for (const char * enabledExtension : m_EnabledInstanceExtensions)
            {
                // Output message if requested extension is not available
                if (std::find(m_SupportedInstanceExtensions.begin(), m_SupportedInstanceExtensions.end(), enabledExtension) == m_SupportedInstanceExtensions.end())
                {
                    VT_CORE_WARN("Enabled instance extension \"{0}\" is not present at instance level", enabledExtension);
                }
                instanceExtensions.push_back(enabledExtension);
            }
        }

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        if (!instanceExtensions.empty())
        {
            if (m_Settings.validation)
            {
                instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);	// SRS - Dependency when VK_EXT_DEBUG_MARKER is enabled
                instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
            instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
        }

        // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
        // Note that on Android this layer requires at least NDK r20
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
        if (m_Settings.validation)
        {
            // Check if this layer is available at instance level
            uint32_t instanceLayerCount;
            vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
            std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
            vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
            bool validationLayerPresent = false;
            for (VkLayerProperties layer : instanceLayerProperties) {
                if (strcmp(layer.layerName, validationLayerName) == 0) {
                    validationLayerPresent = true;
                    break;
                }
            }
            if (validationLayerPresent) {
                instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
                instanceCreateInfo.enabledLayerCount = 1;
            } else {
                VT_CORE_INFO("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
            }
        }
        return vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance);
    }

    void Application::RenderFrame()
    {
        // TODO: Need fence to synch
        Application::PrepareFrame();
        m_SubmitInfo.commandBufferCount = 1;
        m_SubmitInfo.pCommandBuffers = &m_DrawCmdBuffers[m_CurrentBuffer];
        vkQueueSubmit(m_Queue, 1, &m_SubmitInfo, VK_NULL_HANDLE);
        Application::SubmitFrame();
    }

    std::string Application::GetWindowTitle()
    {
        // TODO: Fix
        std::string device{m_DeviceProperties.deviceName};
        std::string windowTitle;
        windowTitle = m_Name + " - " + device;
        if (!m_Settings.overlay)
        {
            windowTitle += " - " + std::to_string(m_FrameCounter) + " fps";
        }
        return windowTitle;
    }

    void Application::CreateCommandBuffers()
    {
        // Create one command buffer for each swap chain image and reuse for rendering
        m_DrawCmdBuffers.resize(m_SwapChain.imageCount);

        VkCommandBufferAllocateInfo cmdBufAllocateInfo = INITIALIZERS::commandBufferAllocateInfo(m_CmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                        static_cast<uint32_t>(m_DrawCmdBuffers.size()));
        if (vkAllocateCommandBuffers(m_Device, &cmdBufAllocateInfo, m_DrawCmdBuffers.data()) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to allocate command buffer");
    }

    void Application::DestroyCommandBuffers()
    {
        vkFreeCommandBuffers(m_Device, m_CmdPool, static_cast<uint32_t>(m_DrawCmdBuffers.size()), m_DrawCmdBuffers.data());
    }

    void Application::CreatePipelineCache()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        if (vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to create pipeline cache");
    }

    void Application::Prepare()
    {
        if (m_VulkanDevice->enableDebugMarkers)
        {
            // TODO: Debug marker
        }
        InitSwapchain();
        CreateCommandPool();
        SetupSwapChain();
        CreateCommandBuffers();
        CreateSynchronizationPrimitives();
        SetupDepthStencil();
        SetupRenderPass();
        CreatePipelineCache();
        SetupFrameBuffer();
        if (m_Settings.overlay)
        {
            // TODO: UI overlay
        }
    }

    VkPipelineShaderStageCreateInfo Application::LoadShader(const std::string &fileName, VkShaderStageFlagBits stage)
    {
        // TODO: separate, and make it into Renderer API
        VkPipelineShaderStageCreateInfo shaderStage{};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = stage;
        shaderStage.module = TOOLS::loadShader(fileName.c_str(), m_Device);
        shaderStage.pName = "main";
        VT_CORE_ASSERT(shaderStage.module != VK_NULL_HANDLE, "Fail to load shader file");
        m_ShaderModules.push_back(shaderStage.module);
        return shaderStage;
    }

    void Application::NextFrame()
    {
        // TODO: camera must be included in client
        if (m_ViewUpdated)
        {
            m_ViewUpdated = false;
            ViewChanged();
        }

        Render();
        m_FrameCounter++;

//        frameTimer = (float)tDiff / 1000.0f;
//        camera.update(frameTimer);
//        if (camera.moving())
//        {
//            viewUpdated = true;
//        }

        // TODO: remove
        UpdateOverlay();
    }

    void Application::RenderLoop()
    {
        // TODO: Use this function in Run()
        if (m_Prepared)
            NextFrame();

        // Flush device to make sure all resources can be freed
//        if (m_Device != VK_NULL_HANDLE) {
//            vkDeviceWaitIdle(m_Device);
//        }
    }

    void Application::UpdateOverlay()
    {
        if (!m_Settings.overlay)
            return;

        // TODO: ImGui UI overlay
    }

    void Application::PrepareFrame()
    {
        // Acquire the next image form the swap chain
        VkResult result = m_SwapChain.acquireNextImage(m_Semaphores.presentComplete, &m_CurrentBuffer);
        // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
        // SRS - If no longer optimal (VK_SUBOPTIMAL_KHR), wait until submitFrame() in case number of swapchain images will change on resize
        if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
        {
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
                WindowResized();
            return;
        } else
        {
            if (result != VK_SUCCESS)
                VT_CORE_ASSERT(false, "Fail to acquire next image");
        }
    }

    void Application::SubmitFrame()
    {
        VkResult result = m_SwapChain.queuePresent(m_Queue, m_CurrentBuffer, m_Semaphores.renderComplete);
        // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
        if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
        {
            WindowResized();
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
                return;
        } else
        {
            if (result != VK_SUCCESS)
                VT_CORE_ASSERT(false, "Fail to present frame");
        }
        vkQueueWaitIdle(m_Queue);
    }

    void Application::ViewChanged() {}

    void Application::KeyPressed(uint32_t) {}

    void Application::MouseMoved(double x, double y, bool & handled) {}

    void Application::BuildCommandBuffers() {}

    void Application::CreateSynchronizationPrimitives()
    {
        // Wait fences to sync command buffer access
        VkFenceCreateInfo fenceCreateInfo = INITIALIZERS::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        m_WaitFences.resize(m_DrawCmdBuffers.size());
        for (auto& fence : m_WaitFences)
        {
            vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &fence);
        }
    }

    void Application::CreateCommandPool()
    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = m_SwapChain.queueNodeIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr, &m_CmdPool);
    }

    void Application::SetupDepthStencil()
    {
        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.format = m_DepthFormat;
        imageCI.extent = { m_Window->GetWidth(), m_Window->GetHeight(), 1 };    // TODO: check
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        vkCreateImage(m_Device, &imageCI, nullptr, &m_DepthStencil.image);
        VkMemoryRequirements memReqs{};
        vkGetImageMemoryRequirements(m_Device, m_DepthStencil.image, &memReqs);

        VkMemoryAllocateInfo memAllloc{};
        memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAllloc.allocationSize = memReqs.size;
        memAllloc.memoryTypeIndex = m_VulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(m_Device, &memAllloc, nullptr, &m_DepthStencil.mem);
        vkBindImageMemory(m_Device, m_DepthStencil.image, m_DepthStencil.mem, 0);

        VkImageViewCreateInfo imageViewCI{};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.image = m_DepthStencil.image;
        imageViewCI.format = m_DepthFormat;
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
        if (m_DepthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
            imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        vkCreateImageView(m_Device, &imageViewCI, nullptr, &m_DepthStencil.view);
    }

    void Application::SetupFrameBuffer()
    {
        VkImageView attachments[2];

        // Depth/Stencil attachment is the same for all frame buffers
        attachments[1] = m_DepthStencil.view;

        VkFramebufferCreateInfo frameBufferCreateInfo{};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.pNext = nullptr;
        frameBufferCreateInfo.renderPass = m_RenderPass;
        frameBufferCreateInfo.attachmentCount = 2;
        frameBufferCreateInfo.pAttachments = attachments;
        frameBufferCreateInfo.width = m_Window->GetWidth();
        frameBufferCreateInfo.height = m_Window->GetHeight();
        frameBufferCreateInfo.layers = 1;

        // Create frame buffers for every swap chain image
        m_FrameBuffers.resize(m_SwapChain.imageCount);
        for (uint32_t i = 0; i < m_FrameBuffers.size(); i++)
        {
            attachments[0] = m_SwapChain.buffers[i].view;
            vkCreateFramebuffer(m_Device, &frameBufferCreateInfo, nullptr, &m_FrameBuffers[i]);
        }
    }

    void Application::SetupRenderPass()
    {
        std::array<VkAttachmentDescription, 2> attachments{};
        // Color attachment
        attachments[0].format = m_SwapChain.colorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // Depth attachment
        attachments[1].format = m_DepthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorReference{};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference{};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies{};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = 0;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
    }

    void Application::GetEnabledFeatures() {}

    void Application::GetEnabledExtensions() {}

    void Application::WindowResize()
    {
        // Called by OnWindowResize
        if (!m_Prepared)
            return;
        m_Prepared = false;
        m_Resized = true;

        // Ensure all operations on the device have been finished before destroying resources
        vkDeviceWaitIdle(m_Device);

        // Recreate swap chain
        // TODO: Window size
        SetupSwapChain();

        // Recreate the frame buffers
        vkDestroyImageView(m_Device, m_DepthStencil.view, nullptr);
        vkDestroyImage(m_Device, m_DepthStencil.image, nullptr);
        vkFreeMemory(m_Device, m_DepthStencil.mem, nullptr);
        SetupDepthStencil();
        for (uint32_t i = 0; i < m_FrameBuffers.size(); i++) {
            vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
        }
        SetupFrameBuffer();

        // TODO: UI overlay

        // Command buffers need to be recreated as they may store
        // references to the recreated frame buffer
        DestroyCommandBuffers();
        CreateCommandBuffers();
        BuildCommandBuffers();

        // SRS - Recreate fences in case number of swap chain images has changed on resize
        for (auto& fence : m_WaitFences)
        {
            vkDestroyFence(m_Device, fence, nullptr);
        }
        CreateSynchronizationPrimitives();

        vkDeviceWaitIdle(m_Device);

        // TODO: Camera aspect ratio

        // Notify derived class
        WindowResized();
        ViewChanged();

        m_Prepared = true;
    }

    void Application::HandleMouseMove(int32_t x, int32_t y)
    {
        // TODO: Implement in client
    }

    void Application::WindowResized()
    {
        // TODO: Implement in client
    }

    void Application::InitSwapchain()
    {
        //Since we use GLFW, platformHandle and platformWindow are not work
        m_SwapChain.initSurface(nullptr, nullptr);
    }

    void Application::SetupSwapChain()
    {
        m_SwapChain.create(m_Window->GetWidth(), m_Window->GetHeight(), m_Settings.vsync, m_Settings.fullscreen);
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer* layer)
    {
        m_LayerStack.PushOverlay(layer);
        layer->OnAttach();
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::Run()
    {
        while (m_Running)
        {
            auto time = static_cast<float>(glfwGetTime());
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            if (!m_Minimized)
            {
                for (auto layer : m_LayerStack)
                {
                    layer->OnUpdate(timestep);
                }

                m_ImGuiLayer->Begin();
                for (auto layer : m_LayerStack)
                {
                    layer->OnImGuiRender();
                }
                m_ImGuiLayer->End();
            }

            // TODO: separate
            RenderLoop();

            m_Window->OnUpdate();
        }
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(VT_BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(VT_BIND_EVENT_FN(Application::OnWindowResize));

        // TODO: Handle events
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.Handled)
                break;
            (*it)->OnEvent(e);
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent &e)
    {
        if (e.GetWidth() == 0 || e.GetHeight())
        {
            m_Minimized = true;
            return false;
        }

        m_Minimized = false;

        WindowResize();

        return false;
    }

}


