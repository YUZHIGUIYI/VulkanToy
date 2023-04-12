//
// Created by ZHIKANG on 2023/4/3.
//

#pragma once

#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/AssetSystem/AsyncUploader.h>

namespace VT
{
    class AssetSystem : public RuntimeModuleInterface
    {
    protected:
        std::filesystem::path m_projectAssetBinFolderPath;

        std::vector<Ref<AssetLoadTask>> m_uploadTasks;
    public:
        [[nodiscard]] const auto& getProjectBinFolderPath() const
        {
            return m_projectAssetBinFolderPath;
        }

    public:
        AssetSystem(const std::string &name = "AssetSystem");
        ~AssetSystem() = default;

        void addUploadTask(Ref<AssetLoadTask> inTask);
        void flushUploadTask();
        void init() override;
        void release() override;
        virtual void tick(const RuntimeModuleTickData &tickData) override;

    private:
        void engineAssetInit();
    };

    using AssetSystemHandle = Singleton<AssetSystem>;
}
