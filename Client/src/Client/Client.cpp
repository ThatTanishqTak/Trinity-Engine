#include "Client/ClientLayer.h"
#include "Engine/Core/EntryPoint.h"

class ClientApp : public Engine::Application
{
public:
    ClientApp()
    {
        PushLayer(new ClientLayer());
    }

    ~ClientApp() override = default;
};

namespace Engine
{
    Application* CreateApplication()
    {
        return new ClientApp();
    }
}