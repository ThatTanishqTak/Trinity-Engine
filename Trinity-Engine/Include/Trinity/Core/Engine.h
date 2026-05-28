#pragma once

#include <memory>
#include <string>

#include <Trinity/Core/Timestep.h>

namespace Trinity
{
    class IPlatform;

    class Engine
    {
    public:
        Engine();
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        bool Initialize(const std::string& applicationName);
        void Update(Timestep timestep);
        void Shutdown();

        IPlatform& GetPlatform() { return *m_Platform; }
        bool HasPlatform() const { return m_Platform != nullptr; }

    private:
        bool m_Initialized = false;
        std::unique_ptr<IPlatform> m_Platform;
    };
}