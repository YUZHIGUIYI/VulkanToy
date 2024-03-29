//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    constexpr size_t GAssetTextureChannels = 4;

    enum class TextureType : uint8_t
    {
        Albedo, Normal, Ao, Metallic, Roughness, Cube, BRDFLUT, Compute, Irradiance, Prefiltered, HDR
    };

    enum class AssetType
    {
        StaticMesh = 0, Texture, Material, Max
    };

    class AssetBinInterface
    {
    protected:
        UUID m_uuid;

    public:
        AssetBinInterface() = default;
        AssetBinInterface(UUID uuid, const std::string &name) : m_uuid(uuid) {}

        [[nodiscard]] const UUID& getBinUUID() const { return m_uuid; }

        virtual AssetType getAssetType() const = 0;
    };

    class GPUAssetInterface
    {
    protected:
        UUID m_uuid;
        bool m_isPersistent;
        std::atomic<bool> m_isAsyncLoading{ true };

    public:
        explicit GPUAssetInterface(bool isPersistent)
                : m_uuid(buildUUID()), m_isPersistent(isPersistent)
        {

        }

        virtual ~GPUAssetInterface() = default;

        bool isAssetLoading() const { return m_isAsyncLoading.load(); };

        bool isAssetReady() const { return !m_isAsyncLoading.load(); }

        bool isPersistent() const { return m_isPersistent; }

        UUID getUUID() const { return m_uuid; }

        void setAsyncLoadState(bool state) { m_isAsyncLoading.store(state); }

        virtual size_t getSize() const = 0;
    };
}
