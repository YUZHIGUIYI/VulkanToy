//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <vulkan/vulkan.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace VT
{
    class Log : public DisableCopy
    {
    public:
        Log();

        Ref<spdlog::logger> s_CoreLogger;
        Ref<spdlog::logger> s_ClientLogger;
    };

    using LogSystem = Singleton<Log>;
}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
    return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
    return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
    return os << glm::to_string(quaternion);
}

// Core log macros
#define VT_CORE_TRACE(...)		::VT::LogSystem::Get()->s_CoreLogger->trace(__VA_ARGS__)
#define VT_CORE_INFO(...)		::VT::LogSystem::Get()->s_CoreLogger->info(__VA_ARGS__)
#define VT_CORE_WARN(...)		::VT::LogSystem::Get()->s_CoreLogger->warn(__VA_ARGS__)
#define VT_CORE_ERROR(...)		::VT::LogSystem::Get()->s_CoreLogger->error(__VA_ARGS__)
#define VT_CORE_CRITICAL(...)	::VT::LogSystem::Get()->s_CoreLogger->critical(__VA_ARGS__); throw std::runtime_error("VulkanToy fatal!")

// Client log macros
#define VT_TRACE(...)			::VT::LogSystem::Get()->s_ClientLogger->trace(__VA_ARGS__)
#define VT_INFO(...)			::VT::LogSystem::Get()->s_ClientLogger->info(__VA_ARGS__)
#define VT_WARN(...)			::VT::LogSystem::Get()->s_ClientLogger->warn(__VA_ARGS__)
#define VT_ERROR(...)			::VT::LogSystem::Get()->s_ClientLogger->error(__VA_ARGS__)
#define VT_CRITICAL(...)		::VT::LogSystem::Get()->s_ClientLogger->critical(__VA_ARGS__)

namespace VT
{
    inline void RHICheck(VkResult err)
    {
        if (err)
        {
            VT_CORE_CRITICAL("Check error: {}", toString(err));
        }
    }

    inline void RHICheck(decltype(VK_NULL_HANDLE) handle)
    {
        if (handle == VK_NULL_HANDLE)
        {
            VT_CORE_CRITICAL("Handle is empty");
        }
    }
}