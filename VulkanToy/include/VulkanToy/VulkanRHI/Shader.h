//
// Created by ZHIKANG on 2023/3/30.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    class ShaderCache
    {
    public:
        VkShaderModule getShader(const std::string &filePath, bool isReload = false);

        void init();
        void release();

    private:
        std::unordered_map<std::string, VkShaderModule> m_shaderModuleContainer;
        void releaseShaderModule(VkShaderModule shader);
    };
}
