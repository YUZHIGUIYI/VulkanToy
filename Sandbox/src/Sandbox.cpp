//
// Created by ZZK on 2023/3/1.
//


#include <Sandbox/Sandbox.h>

namespace VT
{
    Editor* const GEditor = new Editor();

    void Editor::init()
    {

    }

    void Editor::run()
    {
        Launcher::init();

        Launcher::run();

        Launcher::release();
    }

    void Editor::release()
    {

    }
}