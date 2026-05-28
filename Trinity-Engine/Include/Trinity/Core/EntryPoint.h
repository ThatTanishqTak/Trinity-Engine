#pragma once

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Application.h>

extern Trinity::Application* Trinity::CreateApplication(Trinity::CommandLineArgs args);

int main(int argc, char** argv)
{
    Trinity::Log::Initialize();

    Trinity::Application* l_Application = Trinity::CreateApplication({ argc, argv });

    l_Application->Run();

    delete l_Application;

    Trinity::Log::Shutdown();

    return 0;
}