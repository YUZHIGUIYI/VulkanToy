//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/CommandLineParser.h>
#include <VulkanToy/Core/VulkanInitializers.h>
#include <VulkanToy/Core/VulkanBuffer.h>
#include <VulkanToy/Core/VulkanDevice.h>
#include <VulkanToy/Core/VulkanSwapChain.h>

#include <VulkanToy/Core/Window.h>
#include <VulkanToy/Core/Timestep.h>
#include <VulkanToy/Events/Event.h>
#include <VulkanToy/Events/ApplicationEvent.h>
#include <VulkanToy/ImGui/ImGuiLayer.h>
#include <VulkanToy/Core/LayerStack.h>


int main(int argc, char** argv);

namespace VT
{
    struct ApplicationCommandLineArgs
    {
        int count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            return Args[index];
        }
    };

    class Application
    {
    private:
        bool m_Resizing = false;
        std::string shaderDir = "glsl";

    protected:
        ImGuiLayer* m_ImGuiLayer;

    private:
        ApplicationCommandLineArgs m_CommandLineArgs;
        Scope<Window> m_Window;
        LayerStack m_LayerStack;

        bool m_Running = true;

        float m_LastFrameTime = 0.0f;

        std::string m_Name;

    private:
        std::string GetWindowTitle();

        void WindowResize();
        void HandleMouseMove(int32_t x, int32_t y);
        void NextFrame();
        void UpdateOverlay();
        void CreatePipelineCache();
        void CreateCommandPool();
        void CreateSynchronizationPrimitives();
        void InitSwapchain();
        void SetupSwapChain();
        void CreateCommandBuffers();
        void DestroyCommandBuffers();

        // TODO: change it to protected
    public:
        // Frame counter to display fps
        uint32_t m_FrameCounter = 0;

        // Returns the path to the root of the glsl or hlsl shader directory.
        std::string GetShadersPath() const;

        // Vulkan instance, stores all per-application states
        VkInstance m_Instance;
        std::vector<std::string> m_SupportedInstanceExtensions;
        // Physical device (GPU) that Vulkan will use
        VkPhysicalDevice m_PhysicalDevice;
        // Stores physical device properties (for e.g. checking device limits)
        VkPhysicalDeviceProperties m_DeviceProperties;
        // Stores the features available on the selected physical device (for e.g. checking if a feature is available)
        VkPhysicalDeviceFeatures m_DeviceFeatures;
        // Stores all available memory (type) properties for the physical device
        VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;
        /** @brief Set of physical device features to be enabled for this example (must be set in the derived constructor) */
        VkPhysicalDeviceFeatures m_EnabledFeatures{};
        /** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
        std::vector<const char*> m_EnabledDeviceExtensions;
        std::vector<const char*> m_EnabledInstanceExtensions;
        /** @brief Optional pNext structure for passing extension structures to device creation */
        void* m_DeviceCreatepNextChain = nullptr;
        /** @brief Logical device, application's view of the physical device (GPU) */
        VkDevice m_Device;
        // Handle to the device graphics queue that command buffers are submitted to
        VkQueue m_Queue;
        // Depth buffer format (selected during Vulkan initialization)
        VkFormat m_DepthFormat;
        // Command buffer pool
        VkCommandPool m_CmdPool;
        /** @brief Pipeline stages used to wait at for graphics queue submissions */
        VkPipelineStageFlags m_SubmitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // Contains command buffers and semaphores to be presented to the queue
        VkSubmitInfo m_SubmitInfo;
        // Command buffers used for rendering
        std::vector<VkCommandBuffer> m_DrawCmdBuffers;
        // Global render pass for frame buffer writes
        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        // List of available frame buffers (same as number of swap chain images)
        std::vector<VkFramebuffer> m_FrameBuffers;
        // Active frame buffer index
        uint32_t m_CurrentBuffer = 0;
        // Descriptor set pool
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        // List of shader modules created (stored for cleanup)
        std::vector<VkShaderModule> m_ShaderModules;
        // Pipeline cache object
        VkPipelineCache m_PipelineCache;
        // Wraps the swap chain to present images (framebuffers) to the windowing system
        VulkanSwapChain m_SwapChain;
        // Synchronization semaphores
        struct {
            // Swap chain image presentation
            VkSemaphore presentComplete;
            // Command buffer submission and execution
            VkSemaphore renderComplete;
        } m_Semaphores;
        std::vector<VkFence> m_WaitFences;

    public:
        bool m_Prepared = false;
        bool m_Resized = false;
        bool m_ViewUpdated = false;

        CommandLineParser m_CommandLineParser;

        /** @brief Last frame time measured using a high performance timer (if available) */
        float m_FrameTimer = 1.0f;

        /** @brief Encapsulated physical and logical vulkan device */
        VulkanDevice *m_VulkanDevice;

        /** @brief Example settings that can be changed e.g. by command line arguments */
        struct Settings {
            /** @brief Activates validation layers (and message output) when set to true */
            bool validation = false;
            /** @brief Set to true if fullscreen mode has been requested via command line */
            bool fullscreen = false;
            /** @brief Set to true if v-sync will be forced for the swapchain */
            bool vsync = false;
            /** @brief Enable UI overlay */
            bool overlay = true;
        } m_Settings;

        VkClearColorValue m_DefaultClearColor{ { 0.025f, 0.025f, 0.025f, 1.0f } };

        static std::vector<const char*> s_Args;

        // Defines a frame rate independent timer value clamped from -1.0...1.0
        // For use in animations, rotations, etc.
        float m_Timer = 0.0f;
        // Multiplier for speeding up (or slowing down) the global timer
        float m_TimerSpeed = 0.25f;
        bool m_Paused = false;

        uint32_t m_ApiVersion = VK_API_VERSION_1_3;

        struct {
            VkImage image;
            VkDeviceMemory mem;
            VkImageView view;
        } m_DepthStencil;

        struct {
            glm::vec2 axisLeft = glm::vec2(0.0f);
            glm::vec2 axisRight = glm::vec2(0.0f);
        } m_GamePadState;

    public:
        /** @brief Setup the vulkan instance, enable required extensions and connect to the physical device (GPU) */
        bool InitVulkan();

        /** @brief (Virtual) Creates the application wide Vulkan instance */
        virtual VkResult CreateInstance(bool enableValidation);
        /** @brief (Pure virtual) Render function to be implemented by the sample application */
        virtual void Render() = 0;
        /** @brief (Virtual) Called when the camera view has changed */
        virtual void ViewChanged();
        /** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
        virtual void KeyPressed(uint32_t);
        /** @brief (Virtual) Called after the mouse cursor moved and before internal events (like camera rotation) is handled */
        virtual void MouseMoved(double x, double y, bool &handled);
        /** @brief (Virtual) Called when the window has been resized, can be used by the sample application to recreate resources */
        virtual void WindowResized();
        /** @brief (Virtual) Called when resources have been recreated that require a rebuild of the command buffers (e.g. frame buffer), to be implemented by the sample application */
        virtual void BuildCommandBuffers();
        /** @brief (Virtual) Setup default depth and stencil views */
        virtual void SetupDepthStencil();
        /** @brief (Virtual) Setup default framebuffers for all requested swapchain images */
        virtual void SetupFrameBuffer();
        /** @brief (Virtual) Setup a default renderpass */
        virtual void SetupRenderPass();
        /** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
        virtual void GetEnabledFeatures();
        /** @brief (Virtual) Called after the physical device extensions have been read, can be used to enable extensions based on the supported extension listing*/
        virtual void GetEnabledExtensions();

        /** @brief Prepares all Vulkan resources and functions required to run the sample */
        virtual void Prepare();

        /** @brief Loads a SPIR-V shader file for the given shader stage */
        VkPipelineShaderStageCreateInfo LoadShader(const std::string& fileName, VkShaderStageFlagBits stage);

        /** @brief Entry point for the main render loop */
        void RenderLoop();

        /** Prepare the next frame for workload submission by acquiring the next swap chain image */
        void PrepareFrame();
        /** @brief Presents the current image to the swap chain */
        void SubmitFrame();
        /** @brief (Virtual) Default image acquire + submission and command buffer submission function */
        virtual void RenderFrame();

        /** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
        // virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay);

    public:
        Application(const std::string& name = "VulkanToy", ApplicationCommandLineArgs args = ApplicationCommandLineArgs{}, bool enableValidation = false);
        virtual ~Application();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        void Close();

        Window& GetWindow() { return *m_Window; }
        static Application& Get() { return *s_Instance; }
        [[nodiscard]] ApplicationCommandLineArgs GetCommandLineArgs() const { return m_CommandLineArgs; }

    private:
        void Run();
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

    public:
        uint32_t m_Width, m_Height;

    private:
        static Application* s_Instance;
        friend int ::main(int argc, char** argv);
    };

    Application* CreateApplication(ApplicationCommandLineArgs args);
}


























