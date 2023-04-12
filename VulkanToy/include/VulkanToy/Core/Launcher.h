//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Engine.h>

namespace VT
{
    class Launcher
    {
    public:
        static void init();
        static void pushLayer(Layer *layer);
        static void pushOverlay(Layer *layer);
        static void run();
        static void release();
    };
}
