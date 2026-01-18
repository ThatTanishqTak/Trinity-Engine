#pragma once

#include <string>

namespace Engine
{

    class Layer
    {
    public:
        explicit Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnInitialize() {}
        virtual void OnShutdown() {}

        virtual void OnUpdate(float deltaTime) {}
        virtual void OnRender() {}

        /*  
            virtual void OnImGuiRender() {}
            virtual void OnEvent(Event& event) {} 
            
            -------TO BE IMPLEMENTED LATER-------
        */  

        const std::string& GetName() const { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };
}