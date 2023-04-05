//
// Created by ZHIKANG on 2023/4/3.
//

#include <VulkanToy/AssetSystem/MaterialManager.h>

namespace VT
{
    StandardPBRMaterial::StandardPBRMaterial(const UUID &uuid, const std::string &name)
    : MaterialInterface(uuid, name, MaterialType::StandardPBR)
    {

    }
}