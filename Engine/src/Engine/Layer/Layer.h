#pragma once

#include <string>

namespace Engine
{
    class Event;

    class Layer
    {
    public:
        explicit Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnInitialize() {}
        virtual void OnShutdown() {}

        virtual void OnUpdate(float deltaTime) {}
        virtual void OnRender() {}

        virtual void OnEvent(Event& event) {}

        const std::string& GetName() const { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };
}