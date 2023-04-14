//
// Created by ZHIKANG on 2023/4/3.
//

#include <VulkanToy/AssetSystem/AssetSystem.h>
#include <VulkanToy/AssetSystem/TextureManager.h>
#include <VulkanToy/AssetSystem/MeshManager.h>

namespace VT
{
    AssetSystem::AssetSystem(const std::string &name)
    : RuntimeModuleInterface(name)
    {

    }

    void AssetSystem::addUploadTask(Ref<AssetLoadTask> inTask)
    {
        m_uploadTasks.push_back(inTask);
    }

    void AssetSystem::flushUploadTask()
    {
        // TODO: submit and flush tasks
    }

    void AssetSystem::init()
    {
        TextureManager::Get()->init();
        MeshManager::Get()->init();

        // TODO: engine asset init
        engineAssetInit();
    }

    void AssetSystem::engineAssetInit()
    {
        // Mesh upload
        auto GEngineMeshBoxLoad = StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshBox",
                "../data/meshes/Box.obj",
                EngineMeshes::GBoxUUID,
                true);
        auto GEngineMeshSphereLoad = StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshSphere",
                "../data/meshes/Sphere.obj",
                EngineMeshes::GSphereUUID,
                true);

        auto GEngineMeshCerberusLoad = StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshCerberus",
                "../data/meshes/Cerberus.fbx",
                EngineMeshes::GCerberusUUID,
                true);

        auto GEngineMeshSkyboxLoad = StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshSkybox",
                "../data/meshes/Skybox.obj",
                EngineMeshes::GSkyBoxUUID,
                true);

        EngineMeshes::GBoxPtrRef = MeshManager::Get()->getMesh(EngineMeshes::GBoxUUID);
        EngineMeshes::GSpherePtrRef = MeshManager::Get()->getMesh(EngineMeshes::GSphereUUID);
        EngineMeshes::GCerberusRef = MeshManager::Get()->getMesh(EngineMeshes::GCerberusUUID);
        EngineMeshes::GSkyBoxRef = MeshManager::Get()->getMesh(EngineMeshes::GSkyBoxUUID);

        // Texture upload
        auto GEngineImageAoLoad = TextureRawDataLoadTask::buildFromPath(
                "../data/textures/cerberus_A.png",
                EngineImages::GAoImageUUID,
                VK_FORMAT_R8G8B8A8_SRGB,
                TextureType::Albedo);
    }

    void AssetSystem::release()
    {
        // TODO: complete
        TextureManager::Get()->release();
        MeshManager::Get()->release();
    }

    void AssetSystem::tick(const RuntimeModuleTickData &tickData)
    {
        // TODO: submit all task
    }
}