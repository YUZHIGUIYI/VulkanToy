//
// Created by ZZK on 2023/2/6.
//

#pragma once

#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    HelloTriangleApplication app{"HelloVulkan"};

    try
    {
        app.run();
    } catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
