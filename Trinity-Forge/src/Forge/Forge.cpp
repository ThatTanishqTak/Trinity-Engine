#include "Forge/ForgeLayer.h"
#include "Trinity/Core/EntryPoint.h"

#include <memory>

class ForgeApp : public Trinity::Application
{
public:
    ForgeApp(Trinity::ApplicationSpecification& specification) : Trinity::Application(specification)
    {
        PushLayer(std::make_unique<ForgeLayer>());
    }

    ~ForgeApp() override = default;
};

namespace Trinity
{
    Application* CreateApplication(ApplicationSpecification& specification)
    {
        TR_INFO("------- CREATING APPLICATION -------");

        ApplicationSpecification l_Specification = specification;
        l_Specification.Title = "Forge";
        l_Specification.Width = 1280;
        l_Specification.Height = 720;

        TR_INFO("------- APPLICATION CREATED -------");

        return new ForgeApp(l_Specification);
    }
}