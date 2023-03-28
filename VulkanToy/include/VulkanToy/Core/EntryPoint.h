//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Application.h>

int main(int argc, char** argv)
{
    auto app = VT::CreateApplication({ argc, argv });

    app->Prepare();

    app->Run();

    delete app;
}