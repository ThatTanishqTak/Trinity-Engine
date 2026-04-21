#pragma once

#include <string>

namespace Forge
{
    class Panel
    {
    public:
        explicit Panel(std::string name) : m_Name(std::move(name))
        {

        }

        virtual ~Panel() = default;

        virtual void OnInitialize()
        {

        }

        virtual void OnShutdown()
        {

        }

        virtual void OnUpdate(float deltaTime)
        {
            (void)deltaTime;
        }

        virtual void OnPreRender()
        {

        }

        virtual void OnRender() = 0;

        const std::string& GetName() const { return m_Name; }
        bool& GetOpenState() { return m_Open; }
        bool IsOpen() const { return m_Open; }

    protected:
        std::string m_Name;

        bool m_Open = true;
    };
}