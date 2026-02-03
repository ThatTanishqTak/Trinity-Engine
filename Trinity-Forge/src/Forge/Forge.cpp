#include "Forge/ForgeLayer.h"
#include "Trinity/Core/EntryPoint.h"

#include <memory>

class ForgeApp : public Trinity::Application
{
public:
    ForgeApp()
    {
        PushLayer(std::make_unique<ForgeLayer>());
    }

    ~ForgeApp() override = default;
};

namespace Trinity
{
    Application* CreateApplication(/*ApplicationProperties properties*/)
    {
        TR_INFO("------- CREATING APPLICATION -------");

        //properties.Title = "Physics Engine";
        //properties.Width = 1920;
        //properties.Height = 1080;

        TR_INFO("------- APPLICATION CREATED -------");

        return new ForgeApp(/*properties*/);
    }
}