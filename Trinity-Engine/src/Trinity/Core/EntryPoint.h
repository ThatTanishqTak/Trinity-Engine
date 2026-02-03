#pragma once

#include "Trinity/Application/Application.h"
#include "Trinity/Utilities/Utilities.h"
#include "Trinity/Platform/Window.h"

#include <memory>

namespace Trinity
{
    Application* CreateApplication(/*WindowProperties properties*/);
}

int main(int argc, char* argv[])
{
    Trinity::Utilities::Log::Initialize();
    Trinity::Utilities::Time::Initialize();

    std::unique_ptr<Trinity::Application> l_Application(Trinity::CreateApplication());
    
    l_Application->Run();
 
    return 0;
}