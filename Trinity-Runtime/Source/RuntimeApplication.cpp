#include <Trinity/Core/Application.h>
#include <Trinity/Core/EntryPoint.h>
#include <Trinity/Core/Log.h>
#include <Trinity/Core/Event.h>
#include <Trinity/Platform/Events/KeyEvent.h>

namespace Trinity
{
    class RuntimeApplication : public Application
    {
    public:
        RuntimeApplication(const ApplicationSpecification& specification) : Application(specification)
        {

        }

        void OnInitialize() override
        {
            ("Runtime initialized");
        }

        void OnUpdate(Timestep) override
        {

        }

        void OnEvent(Event& event, bool handled) override
        {
            if (handled)
            {
                return;
            }

            EventDispatcher l_Dispatcher(event);
            l_Dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& key)
            {
                ("Key pressed: {}", key.GetKeyCode());
                return false;
            });
        }

        void OnShutdown() override
        {
            ("Runtime shutting down");
        }
    };

    Application* CreateApplication(CommandLineArgs args)
    {
        ApplicationSpecification l_Specification;
        l_Specification.InternalName = "Trinity-Runtime";
        l_Specification.Window.Title = "Runtime";
        l_Specification.Window.Width = 1920;
        l_Specification.Window.Height = 1080;
        l_Specification.Arguments = args;

        return new RuntimeApplication(l_Specification);
    }
}