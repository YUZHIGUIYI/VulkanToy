//
// Created by ZHIKANG on 2023/4/3.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    struct RuntimeModuleTickData
    {
        int32_t windowWidth;
        int32_t windowHeight;

        // Application minimized
        bool isMinimized = false;

        // Application focus
        bool isFocus = false;

        // Application realtime delta time
        float deltaTime = 0.0f;
    };

    class RuntimeModuleInterface
    {
    protected:
        std::string m_moduleName;

    public:
        RuntimeModuleInterface(const std::string &name)
        :   m_moduleName(name)
        {

        }

        virtual ~RuntimeModuleInterface() = default;

        virtual void init() { }
        virtual void tick(const RuntimeModuleTickData &tickData) = 0;
        virtual void release() { }
    };
}
