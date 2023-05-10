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
        // Engine mesh upload
        StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshBox",
                "../data/meshes/Box.obj",
                EngineMeshes::GBoxUUID,
                true);
        StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshSphere",
                "../data/meshes/Sphere.obj",
                EngineMeshes::GSphereUUID,
                true);

        StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshCerberus",
                "../data/meshes/Cerberus.fbx",
                EngineMeshes::GCerberusUUID,
                true);

        StaticMeshRawDataLoadTask::buildFromPath(
                "EngineMeshSkybox",
                "../data/meshes/Skybox.obj",
                EngineMeshes::GSkyBoxUUID,
                true);

        EngineMeshes::GBoxPtrRef = MeshManager::Get()->getMesh(EngineMeshes::GBoxUUID);
        EngineMeshes::GSpherePtrRef = MeshManager::Get()->getMesh(EngineMeshes::GSphereUUID);
        EngineMeshes::GCerberusRef = MeshManager::Get()->getMesh(EngineMeshes::GCerberusUUID);
        EngineMeshes::GSkyBoxRef = MeshManager::Get()->getMesh(EngineMeshes::GSkyBoxUUID);

        // Engine texture upload
        TextureRawDataLoadTask::buildFromPath(
            "../data/textures/cerberus_A.png",
            EngineImages::GAlbedoImageUUID,
            VK_FORMAT_R8G8B8A8_SRGB,
            TextureType::Albedo);

        TextureRawDataLoadTask::buildFromPath(
            "../data/textures/cerberus_N.png",
            EngineImages::GNormalImageUUID,
            VK_FORMAT_R8G8B8A8_UNORM,
            TextureType::Normal);

        TextureRawDataLoadTask::buildFromPath(
            "../data/textures/cerberus_R.png",
            EngineImages::GRoughnessImageUUID,
            VK_FORMAT_R8_UNORM,
            TextureType::Roughness);

        TextureRawDataLoadTask::buildFromPath(
            "../data/textures/cerberus_M.png",
            EngineImages::GMetallicImageUUID,
            VK_FORMAT_R8_UNORM,
            TextureType::Metallic);
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