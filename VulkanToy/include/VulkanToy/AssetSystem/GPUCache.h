//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/AssetSystem/AssetCommon.h>

namespace VT
{
    template <typename ValueType = GPUAssetInterface, typename KeyType = UUID>
    class GPUAssetCache
    {
    protected:
        static_assert(std::is_base_of_v<GPUAssetInterface, ValueType>, "Value type muse be derived from GPUAssetInterface");

        // Cache persistent resource
        std::atomic<size_t> m_persistentAssetSize{ 0 };
        std::unordered_map<KeyType, Ref<ValueType>> m_cachePersistentMap;

        // Mutex for map edit
        std::mutex m_mutex;

        // GPU cache desired capacity
        size_t m_capacity;

        // Elastic space to enable GPU cache no always prune
        size_t m_elasticity;

    public:
        explicit GPUAssetCache(size_t capacity, size_t elasticity)
            : m_capacity(capacity * 1024 * 1024), m_elasticity(elasticity * 1024 * 1024)
        {

        }

        virtual ~GPUAssetCache() = default;

        size_t getCapacity() const { return m_capacity; }

        size_t getElasticity() const { return m_elasticity; }

        size_t getMaxAllowedSize() const { return m_capacity + m_elasticity; }

        size_t getPersistentAssetSize() const { return m_persistentAssetSize.load(); }

        bool contain(const KeyType &key)
        {
            // Need lock when search from weak ptr map
            std::lock_guard<std::mutex> guard(m_mutex);

            if (m_cachePersistentMap.contains(key))
            {
                return true;
            }

            return false;
        }

        void insert(const KeyType &key, Ref<ValueType> value)
        {
            std::lock_guard<std::mutex> guard(m_mutex);

            // Persistent asset
            // TODO: check persistent
            auto it = m_cachePersistentMap.try_emplace(key, value);
            if (it.second)
            {
                m_persistentAssetSize += value->getSize();
            } else
            {
                VT_CORE_ERROR("Try insert persistent asset that already exists");
            }
        }

        Ref<ValueType> tryGet(const KeyType &inKey)
        {
            std::lock_guard<std::mutex> guard(m_mutex);

            if (m_cachePersistentMap.contains(inKey))
            {
                return m_cachePersistentMap[inKey];
            }

            return nullptr;
        }

        void clear()
        {
            std::lock_guard<std::mutex> guard(m_mutex);

            m_cachePersistentMap.clear();
        }
    };
}











