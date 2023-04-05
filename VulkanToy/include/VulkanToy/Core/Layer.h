//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/Events/Event.h>
#include <VulkanToy/Core/RuntimeModule.h>

namespace VT
{
    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(const RuntimeModuleTickData &tickData) {}
        virtual void OnImGuiRender() {}
        virtual void OnEvent(Event& event) {}

        inline const std::string& GetName() const { return m_DebugName; }
    protected:
        std::string m_DebugName;
    };
}
