//
// Created by ZHIKANG on 2023/5/7.
//

#include <VulkanToy/AssetSystem/ImageProcess.h>

#include <stb_image.h>

namespace VT
{
    ImageProcess::ImageProcess(const char *fileName, TextureType textureType)
    {
        if (textureType == TextureType::HDR)
        {
            m_pixels = reinterpret_cast<uint8_t *>(stbi_loadf(fileName, &m_width, &m_height, &m_channels, STBI_rgb_alpha));
            m_channels = 4 * sizeof(float);
        } else if (textureType != TextureType::Metallic && textureType != TextureType::Roughness)
        {
            m_pixels = stbi_load(fileName, &m_width, &m_height, &m_channels, STBI_rgb_alpha);
            m_channels = 4;
        } else
        {
            m_pixels = stbi_load(fileName, &m_width, &m_height, &m_channels, STBI_grey);
            m_channels = 1;
        }
    }

    ImageProcess::~ImageProcess()
    {
        stbi_image_free(m_pixels);
    }
}