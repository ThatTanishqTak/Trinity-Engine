#pragma once

#include "Engine/Application/Application.h"

namespace Engine
{
    Application* CreateApplication();
}

int main()
{
    auto* a_Application = Engine::CreateApplication();

    a_Application->Run();
    
    delete a_Application;

    return 0;
}