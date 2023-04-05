//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy.h>

namespace VT
{
    class Editor final
    {
    public:
        void init();

        void run();

        void release();
    };

    extern Editor* const GEditor;
}
