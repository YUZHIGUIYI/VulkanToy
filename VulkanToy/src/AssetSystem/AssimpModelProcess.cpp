//
// Created by ZHIKANG on 2023/4/2.
//

#include <VulkanToy/AssetSystem/AssimpModelProcess.h>

namespace VT
{
    AssimpModelProcess::AssimpModelProcess(const std::filesystem::path &inPath)
    : folderPath(inPath)
    {

    }

    void AssimpModelProcess::processNode(aiNode *node, const aiScene *scene)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            m_subMeshInfos.push_back(processMesh(mesh, scene));
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    StaticMeshSubMesh AssimpModelProcess::processMesh(aiMesh *mesh, const aiScene *scene)
    {
        StaticMeshSubMesh subMeshInfo{};
        subMeshInfo.indexStartPosition = static_cast<uint32_t>(m_indices.size());
        uint32_t indexOffset = static_cast<uint32_t>(m_vertices.size());

        std::vector<StaticMeshVertex> vertices{};
        std::vector<VertexIndexType> indices{};

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            StaticMeshVertex vertex;

            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;

            if (mesh->mTextureCoords[0])
            {
                vertex.uv.x = mesh->mTextureCoords[0][i].x;
                vertex.uv.y = mesh->mTextureCoords[0][i].y;
            } else
            {
                vertex.uv = glm::vec2{ 0.0f, 0.0f };
            }

            // Tangent
            glm::vec4 tangentVec{};
            tangentVec.x = mesh->mTangents[i].x;
            tangentVec.y = mesh->mTangents[i].y;
            tangentVec.z = mesh->mTangents[i].z;

            glm::vec3 vector{};
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;

            glm::vec3 bitangent{};
            bitangent.x = mesh->mBitangents[i].x;
            bitangent.y = mesh->mBitangents[i].y;
            bitangent.z = mesh->mBitangents[i].z;

            // Tangent sign process
            tangentVec.w = glm::sign(glm::dot(glm::normalize(bitangent), glm::normalize(glm::cross(vertex.normal, vector))));
            vertex.tangent = tangentVec;
            vertices.push_back(vertex);
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; ++j)
            {
                indices.push_back(indexOffset + face.mIndices[j]);
            }
        }

        m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
        m_indices.insert(m_indices.end(), indices.begin(), indices.end());

        subMeshInfo.indexCount = static_cast<uint32_t>(indices.size());

        // AABB bound process
        auto aabbMax = mesh->mAABB.mMax;
        auto aabbMin = mesh->mAABB.mMin;
        auto aabbExt = (aabbMax - aabbMin) * 0.5f;
        auto aabbCenter = aabbExt + aabbMin;
        subMeshInfo.renderBound.extent[0] = aabbExt.x;
        subMeshInfo.renderBound.extent[1] = aabbExt.y;
        subMeshInfo.renderBound.extent[2] = aabbExt.z;
        subMeshInfo.renderBound.origin[0] = aabbCenter.x;
        subMeshInfo.renderBound.origin[1] = aabbCenter.y;
        subMeshInfo.renderBound.origin[2] = aabbCenter.z;
        subMeshInfo.renderBound.radius = glm::distance(
            glm::vec3(aabbMax.x, aabbMax.y, aabbMax.x),
            glm::vec3(aabbCenter.x, aabbCenter.y, aabbCenter.z));

        // Standard PBR texture prepare
        aiString baseColorTextures{};
        aiString normalTextures{};
        aiString specularTextures{};
        aiString aoTextures{};
        aiString emissiveTextures{};

        auto tryFetchTexture = [this](const char *pathIn, bool isSrgb, TextureType texType)
        {
            const auto path = (folderPath / pathIn).string();
            if (m_texturePathMap.contains(texType))
            {
                // TODO
                VT_CORE_WARN("Already exists: {0}", path);
            } else
            {
                m_texturePathMap[texType] = path;
            }
        };

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            static const std::string materialName = "_mat";

            // TODO: Create new material
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                material->GetTexture(aiTextureType_DIFFUSE, 0, &baseColorTextures);
                // TODO: get texture
                tryFetchTexture(baseColorTextures.C_Str(), true, TextureType::DIFFUSE);
            }
            if (material->GetTextureCount(aiTextureType_HEIGHT) > 0)
            {
                material->GetTexture(aiTextureType_HEIGHT, 0, &normalTextures);
                // TODO: get texture
                tryFetchTexture(normalTextures.C_Str(), false, TextureType::HEIGHT);
            }
            if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
            {
                material->GetTexture(aiTextureType_SPECULAR, 0, &specularTextures);
                // TODO: get texture
                tryFetchTexture(specularTextures.C_Str(), false, TextureType::SPECULAR);
            }
            if (material->GetTextureCount(aiTextureType_AMBIENT) > 0)
            {
                material->GetTexture(aiTextureType_AMBIENT, 0, &aoTextures);
                // TODO: get texture
                tryFetchTexture(aoTextures.C_Str(), false, TextureType::AMBIENT);
            }
            if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
            {
                material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveTextures);
                // TODO: get texture
                tryFetchTexture(emissiveTextures.C_Str(), true, TextureType::EMISSIVE);
            }

            // TODO:
        } else
        {
            subMeshInfo.material = {};
        }

        return subMeshInfo;
    }
}









