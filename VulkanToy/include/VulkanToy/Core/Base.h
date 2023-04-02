//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <Pch.h>

#ifdef _DEBUG
    #define VT_DEBUG
#endif

#ifdef VT_DEBUG
    #define VT_DEBUGBREAK() __debugbreak()
    #define VT_ENABLE_ASSERTS
#else
    #define VT_DEBUGBREAK()
#endif

#define VT_EXPAND_MACRO(x) x
#define VT_STRINGIFY_MACRO(x) #x

#ifdef VT_ENABLE_ASSERTS
    // Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
    // provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
    #define VT_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { VT##type##ERROR(msg, __VA_ARGS__); VT_DEBUGBREAK(); } }
    #define VT_INTERNAL_ASSERT_WITH_MSG(type, check, ...) VT_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
    #define VT_INTERNAL_ASSERT_NO_MSG(type, check) VT_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", VT_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

    #define VT_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
    #define VT_INTERNAL_ASSERT_GET_MACRO(...) VT_EXPAND_MACRO( VT_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, VT_INTERNAL_ASSERT_WITH_MSG, VT_INTERNAL_ASSERT_NO_MSG) )

    // Currently accepts at least the condition and one additional parameter (the message) being optional
    #define VT_ASSERT(...) VT_EXPAND_MACRO( VT_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
    #define VT_CORE_ASSERT(...) VT_EXPAND_MACRO( VT_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
    #define VT_ASSERT(...)
    #define VT_CORE_ASSERT(...)
#endif

#define BIT(x) (1 << x)

// C++17 standard
#define VT_BIND_EVENT_FN(fn) [this](auto&&... args) ->decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// For Vulkan RHI
// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000
// Default descriptor pool size
#define DESCRIPTOR_POOL_SIZE 256
// Default maximum color attachment number
#define MAX_COLOR_ATTACHMENTS 8

namespace VT
{
    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T> concept StringStreamable = requires(std::stringstream & ss, const T & value) { ss << value; };

    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <class T> requires StringStreamable<T>
    inline std::string toString(const T& value)
    {
        std::stringstream ss;

        ss << std::fixed << value;
        return ss.str();
    }
}

#include <VulkanToy/Core/DisableCopy.h>
#include <VulkanToy/Core/Singleton.h>
#include <VulkanToy/Core/Log.h>
#include <VulkanToy/Core/UUID.h>




