#pragma once

#include "Trinity/Application/Application.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/Time.h"
#include "Trinity/Platform/Window.h"

#include <memory>

namespace Trinity
{
    Application* CreateApplication(ApplicationSpecification& specification);
}

int main(int argc, char* argv[])
{
    Trinity::Utilities::Log::Initialize();
    Trinity::Utilities::Time::Initialize();

    Trinity::ApplicationSpecification l_Specification;
    std::unique_ptr<Trinity::Application> l_Application(Trinity::CreateApplication(l_Specification));

    l_Application->Run();

    return 0;
}