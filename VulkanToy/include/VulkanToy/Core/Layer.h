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
        Layer(const std::string& name = "Layer") : m_debugName(name) {}
        virtual ~Layer() = default;

        virtual void onAttach() {}
        virtual void onDetach() {}
        virtual void tick(const RuntimeModuleTickData &tickData) {}
        virtual void onImGuiRender() {}
        virtual void onEvent(Event& event) {}

        [[nodiscard]] const std::string& getName() const { return m_debugName; }
    protected:
        std::string m_debugName;
    };
}
