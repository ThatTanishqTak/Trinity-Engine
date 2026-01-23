#pragma once

#include "Engine/Application/Application.h"
#include "Engine/Utilities/Utilities.h"

#include <memory>

namespace Engine
{
    Application* CreateApplication();
}

int main()
{
    Engine::Utilities::Log::Initialize();
    Engine::Utilities::Time::Initialize();

    std::unique_ptr<Engine::Application> l_Application(Engine::CreateApplication());
    
    l_Application->Run();
 
    return 0;
}