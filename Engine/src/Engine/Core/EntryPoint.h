#pragma once

#include "Engine/Application/Application.h"

#include <memory>

namespace Engine
{
    Application* CreateApplication();
}

int main()
{
    std::unique_ptr<Engine::Application> l_Application(Engine::CreateApplication());
    
    l_Application->Run();
 
    return 0;
}