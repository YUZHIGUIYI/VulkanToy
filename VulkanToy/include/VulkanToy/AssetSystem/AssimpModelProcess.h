//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/AssetSystem/MeshMisc.h>
#include <VulkanToy/AssetSystem/AssetCommon.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

namespace VT
{
    struct AssimpModelProcess
    {
        std::filesystem::path folderPath;

        std::vector<StaticMeshSubMesh> m_subMeshInfos{};
        std::vector<StaticMeshVertex> m_vertices{};
        std::vector<VertexIndexType> m_indices{};

        explicit AssimpModelProcess(const std::filesystem::path &inPath);

//        void processNode(aiNode *node, const aiScene *scene);                 // Old method

//        StaticMeshSubMesh processMesh(aiMesh *mesh, const aiScene *scene);    // Old method

        void processMesh(aiMesh *mesh, const aiScene *scene);                   // New method
    };
}




























