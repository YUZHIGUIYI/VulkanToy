//
// Created by ZHIKANG on 2023/5/7.
//

#pragma once

#include <VulkanToy/AssetSystem/AssetCommon.h>

namespace VT
{
    class ImageProcess final
    {
    private:
        int32_t m_width = 0;
        int32_t m_height = 0;
        int32_t m_channels = 0;
        uint8_t* m_pixels = nullptr;

    public:
        explicit ImageProcess(const char * fileName, TextureType textureType);
        ~ImageProcess();

        [[nodiscard]] int32_t getWidth() const { return m_width; }

        [[nodiscard]] int32_t getHeight() const { return m_height; }

        [[nodiscard]] uint8_t* getPixels() const { return m_pixels; }

        [[nodiscard]] VkDeviceSize getImageSize() const { return m_width * m_height * m_channels; }
    };

    static_assert(std::is_same_v<unsigned char, uint8_t>, "unsigned char must be same as uint8_t");
}
