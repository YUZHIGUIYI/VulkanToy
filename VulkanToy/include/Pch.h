//
// Created by ZHIKANG on 2023/3/27.
//

#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <list>
#include <vector>
#include <string>
#include <string_view>
#include <stack>
#include <variant>
#include <utility>
#include <type_traits>
#include <thread>
#include <mutex>
#include <fstream>
#include <exception>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <shared_mutex>

// glm math.
// 0 - glm force compute on radians.
// 1 - glm vulkan depth force 0 to 1.
// 2 - glm enable experimental.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace glm
{
    template<class Archive> void serialize(Archive& archive, glm::vec2&  v) { archive(v.x, v.y); }
    template<class Archive> void serialize(Archive& archive, glm::vec3&  v) { archive(v.x, v.y, v.z); }
    template<class Archive> void serialize(Archive& archive, glm::vec4&  v) { archive(v.x, v.y, v.z, v.w); }
    template<class Archive> void serialize(Archive& archive, glm::ivec2& v) { archive(v.x, v.y); }
    template<class Archive> void serialize(Archive& archive, glm::ivec3& v) { archive(v.x, v.y, v.z); }
    template<class Archive> void serialize(Archive& archive, glm::ivec4& v) { archive(v.x, v.y, v.z, v.w); }
    template<class Archive> void serialize(Archive& archive, glm::uvec2& v) { archive(v.x, v.y); }
    template<class Archive> void serialize(Archive& archive, glm::uvec3& v) { archive(v.x, v.y, v.z); }
    template<class Archive> void serialize(Archive& archive, glm::uvec4& v) { archive(v.x, v.y, v.z, v.w); }
    template<class Archive> void serialize(Archive& archive, glm::dvec2& v) { archive(v.x, v.y); }
    template<class Archive> void serialize(Archive& archive, glm::dvec3& v) { archive(v.x, v.y, v.z); }
    template<class Archive> void serialize(Archive& archive, glm::dvec4& v) { archive(v.x, v.y, v.z, v.w); }
    template<class Archive> void serialize(Archive& archive, glm::mat2&  m) { archive(m[0], m[1]); }
    template<class Archive> void serialize(Archive& archive, glm::dmat2& m) { archive(m[0], m[1]); }
    template<class Archive> void serialize(Archive& archive, glm::mat3&  m) { archive(m[0], m[1], m[2]); }
    template<class Archive> void serialize(Archive& archive, glm::mat4&  m) { archive(m[0], m[1], m[2], m[3]); }
    template<class Archive> void serialize(Archive& archive, glm::dmat4& m) { archive(m[0], m[1], m[2], m[3]); }
    template<class Archive> void serialize(Archive& archive, glm::quat&  q) { archive(q.x, q.y, q.z, q.w); }
    template<class Archive> void serialize(Archive& archive, glm::dquat& q) { archive(q.x, q.y, q.z, q.w); }
}

// vulkan
#include <vulkan/vulkan.h>

// Use glfw as window manager library.
#include <GLFW/glfw3.h>

// Use ImGui as UI library
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

// Vulkan memory allocator
#include <vma/vk_mem_alloc.h>

// UUID - see https://github.com/mariusbancila/stduuid
#define UUID_SYSTEM_GENERATOR
#include <uuid.h>


