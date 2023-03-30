//
// Created by ZHIKANG on 2023/3/30.
//

#include <VulkanToy/VulkanRHI/Shader.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    VkShaderModule createShaderModule(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            VT_CORE_ERROR("Fail to open shader file: {0}", filename);
            throw std::runtime_error("Fail to open shader file");
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> codes(fileSize);
        file.seekg(0);
        file.read(codes.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = codes.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(codes.data());
        VkShaderModule shaderModule;
        RHICheck(vkCreateShaderModule(VulkanRHI::Device, &shaderModuleCreateInfo, nullptr, &shaderModule));

        return shaderModule;
    }

    VkShaderModule ShaderCache::getShader(const std::string &filePath, bool isReload)
    {
        static const std::string shaderFileDirectory = "../data/shaders/spirv/";
        std::string path = shaderFileDirectory + filePath;

        const bool isExist = m_shaderModuleContainer.contains(path);
        if (isExist && isReload)
        {
            releaseShaderModule(m_shaderModuleContainer[path]);
        }

        const bool isLoad = isReload || (!isExist);
        if (isLoad)
        {
            m_shaderModuleContainer[path] = createShaderModule(path);
        }

        return m_shaderModuleContainer[path];
    }

    void ShaderCache::init()
    {
        // TODO
    }

    void ShaderCache::release()
    {
        for (auto& shaders : m_shaderModuleContainer)
        {
            releaseShaderModule(shaders.second);
        }
        m_shaderModuleContainer.clear();
    }

    void ShaderCache::releaseShaderModule(VkShaderModule shader)
    {
        vkDestroyShaderModule(VulkanRHI::Device, shader, nullptr);
    }
}