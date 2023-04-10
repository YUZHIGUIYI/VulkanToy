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
            StaticMeshVertex vertex{};

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

        subMeshInfo.material = {};

        return subMeshInfo;
    }
}









