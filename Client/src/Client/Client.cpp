#include "Client/ClientLayer.h"
#include "Engine/Core/EntryPoint.h"

#include <memory>

class ClientApp : public Engine::Application
{
public:
    ClientApp()
    {
        PushLayer(std::make_unique<ClientLayer>());
    }

    ~ClientApp() override = default;
};

namespace Engine
{
    Application* CreateApplication(/*ApplicationProperties properties*/)
    {
        //properties.Title = "Physics Engine";
        //properties.Width = 1920;
        //properties.Height = 1080;

        return new ClientApp(/*properties*/);
    }
}