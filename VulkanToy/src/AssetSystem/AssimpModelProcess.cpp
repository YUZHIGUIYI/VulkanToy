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

//    void AssimpModelProcess::processNode(aiNode *node, const aiScene *scene)
//    {
//        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
//        {
//            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//            m_subMeshInfos.push_back(processMesh(mesh, scene));
//        }
//
//        for (uint32_t i = 0; i < node->mNumChildren; ++i)
//        {
//            processNode(node->mChildren[i], scene);
//        }
//    }

//    StaticMeshSubMesh AssimpModelProcess::processMesh(aiMesh *mesh, const aiScene *scene)
//    {
//        VT_CORE_ASSERT(mesh->HasPositions(), "The model file does not have position information");
//        VT_CORE_ASSERT(mesh->HasNormals(), "The model file does not have normal information");
//
//        StaticMeshSubMesh subMeshInfo{};
//        subMeshInfo.indexStartPosition = static_cast<uint32_t>(m_indices.size());
//        uint32_t indexOffset = static_cast<uint32_t>(m_vertices.size());
//
//        std::vector<StaticMeshVertex> vertices{};
//        std::vector<VertexIndexType> indices{};
//
//        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
//        {
//            StaticMeshVertex vertex{};
//
//            vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
//            vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
//
//            if (mesh->HasTextureCoords(0))
//            {
//                vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
//            } else
//            {
//                vertex.uv = glm::vec2{ 0.0f, 0.0f };
//            }
//
//            // Tangent and bitangent
//            if (mesh->HasTangentsAndBitangents())
//            {
//                glm::vec4 tangentVec = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f };
//
//                glm::vec3 vector = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
//
//                glm::vec3 bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
//
//                // Tangent sign process
//                tangentVec.w = glm::sign(
//                        glm::dot(glm::normalize(bitangent), glm::normalize(glm::cross(vertex.normal, vector))));
//
//                vertex.tangent = tangentVec;
//            }
//
//            vertices.push_back(vertex);
//        }
//
//        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
//        {
//            aiFace face = mesh->mFaces[i];
//            for (uint32_t j = 0; j < face.mNumIndices; ++j)
//            {
//                indices.push_back(indexOffset + face.mIndices[j]);
//            }
//        }
//
//        m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
//        m_indices.insert(m_indices.end(), indices.begin(), indices.end());
//
//        subMeshInfo.indexCount = static_cast<uint32_t>(indices.size());
//
//        // AABB bound process
//        auto aabbMax = mesh->mAABB.mMax;
//        auto aabbMin = mesh->mAABB.mMin;
//        auto aabbExt = (aabbMax - aabbMin) * 0.5f;
//        auto aabbCenter = aabbExt + aabbMin;
//        subMeshInfo.renderBound.extent[0] = aabbExt.x;
//        subMeshInfo.renderBound.extent[1] = aabbExt.y;
//        subMeshInfo.renderBound.extent[2] = aabbExt.z;
//        subMeshInfo.renderBound.origin[0] = aabbCenter.x;
//        subMeshInfo.renderBound.origin[1] = aabbCenter.y;
//        subMeshInfo.renderBound.origin[2] = aabbCenter.z;
//        subMeshInfo.renderBound.radius = glm::distance(
//            glm::vec3(aabbMax.x, aabbMax.y, aabbMax.x),
//            glm::vec3(aabbCenter.x, aabbCenter.y, aabbCenter.z));
//
//        subMeshInfo.material = {};
//
//        return subMeshInfo;
//    }

    void AssimpModelProcess::processMesh(aiMesh *mesh, const aiScene *scene)
    {
        VT_CORE_ASSERT(mesh->HasPositions(), "The model file does not have position information");
        VT_CORE_ASSERT(mesh->HasNormals(), "The model file does not have normal information");

        m_vertices.reserve(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            StaticMeshVertex vertex{};

            vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

            if (mesh->HasTextureCoords(0))
            {
                vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            } else
            {
                vertex.uv = glm::vec2{ 0.0f, 0.0f };
            }

            // Tangent and bitangent
            if (mesh->HasTangentsAndBitangents())
            {
//                glm::vec4 tangentVec = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f };
//
//                glm::vec3 vector = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
//
//                glm::vec3 bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
//
//                // Tangent sign process
//                tangentVec.w = glm::sign(
//                        glm::dot(glm::normalize(bitangent), glm::normalize(glm::cross(vertex.normal, vector))));
//
//                vertex.tangent = tangentVec;
                vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
            }

            m_vertices.push_back(vertex);
        }

        m_indices.reserve(3 * mesh->mNumFaces);
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            VT_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Ensure the model is triangle face");
            m_indices.push_back(mesh->mFaces[i].mIndices[0]);
            m_indices.push_back(mesh->mFaces[i].mIndices[1]);
            m_indices.push_back(mesh->mFaces[i].mIndices[2]);
        }

        StaticMeshSubMesh subMeshInfo{};
        subMeshInfo.indexCount = static_cast<uint32_t>(m_indices.size());

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

        m_subMeshInfos.emplace_back(subMeshInfo);
    }
}









