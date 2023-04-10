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
                "../data/meshes/cerberus.fbx",
                EngineMeshes::GCerberusUUID,
                true);

        EngineMeshes::GBoxPtrRef = MeshManager::Get()->getMesh(EngineMeshes::GBoxUUID);
        EngineMeshes::GSpherePtrRef = MeshManager::Get()->getMesh(EngineMeshes::GSphereUUID);
        EngineMeshes::GCerberusRef = MeshManager::Get()->getMesh(EngineMeshes::GCerberusUUID);
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