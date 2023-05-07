//
// Created by ZHIKANG on 2023/4/13.
//

#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/Renderer/PreprocessPass.h>

namespace VT
{
    class SkyboxPass final
    {
    private:
        VkPipeline skyboxPipeline = VK_NULL_HANDLE;
        VkPipelineLayout skyboxPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout skyboxDescriptorSetLayout = VK_NULL_HANDLE;

    private:
        void setupDescriptorLayout();
        void setupPipeline(VkRenderPass renderPass);

    public:
        void init(VkRenderPass renderPass);

        void release();

        void onRenderTick(VkCommandBuffer cmd);

        [[nodiscard]] const VkPipeline& getPipeline() const { return skyboxPipeline; }
        [[nodiscard]] const VkPipelineLayout& getPipelineLayout() const { return skyboxPipelineLayout; }
    };

    class PBRPass final
    {
    private:
        VkPipeline pbrPipeline = VK_NULL_HANDLE;
        VkPipelineLayout pbrPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout pbrDescriptorSetLayout = VK_NULL_HANDLE;

    private:
        void setupDescriptorLayout();
        void setupPipeline(VkRenderPass renderPass);

    public:
        void init(VkRenderPass renderPass);

        void release();

        void onRenderTick(VkCommandBuffer cmd);

        [[nodiscard]] const VkPipeline& getPipeline() const { return pbrPipeline; }
        [[nodiscard]] const VkPipelineLayout& getPipelineLayout() const { return pbrPipelineLayout; }
    };

    using PassInterface = std::variant<Ref<PreprocessPass>, Ref<SkyboxPass>, Ref<PBRPass>>;
    using PassCollector = std::vector<PassInterface>;
}
