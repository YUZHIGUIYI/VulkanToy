//
// Created by ZHIKANG on 2023/4/5.
//

#include <VulkanToy/Core/Launcher.h>

namespace VT
{
    void Launcher::init()
    {
        GEngine->init();
    }

    void Launcher::run()
    {
        GEngine->run();
    }

    void Launcher::release()
    {
        GEngine->release();
    }

    void Launcher::pushLayer(Layer *layer)
    {
        GEngine->pushLayer(layer);
    }

    void Launcher::pushOverlay(Layer *layer)
    {
        GEngine->pushOverlay(layer);
    }
}