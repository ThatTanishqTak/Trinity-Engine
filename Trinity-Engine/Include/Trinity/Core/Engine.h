#pragma once

#include <memory>
#include <string>

#include <Trinity/Core/Timestep.h>

namespace Trinity
{
    class IPlatform;
    class VulkanDevice;

    struct NativeWindowHandle;

    class Engine
    {
    public:
        Engine();
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        bool Initialize(const std::string& applicationName);
        bool InitializeRenderer(const NativeWindowHandle& window, const std::string& applicationName);
        void Update(Timestep timestep);
        void Shutdown();

        IPlatform& GetPlatform() { return *m_Platform; }
        bool HasPlatform() const { return m_Platform != nullptr; }

        VulkanDevice& GetDevice() { return *m_Device; }
        bool HasDevice() const { return m_Device != nullptr; }

    private:
        bool m_Initialized = false;

        std::unique_ptr<IPlatform> m_Platform;
        std::unique_ptr<VulkanDevice> m_Device;
    };
}