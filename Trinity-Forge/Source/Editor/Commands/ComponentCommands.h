#pragma once

#include <cstdint>
#include <string>

#include <Editor/ICommand.h>
#include <Editor/Commands/EntityCommands.h>

#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>

namespace Trinity
{
    template<typename T>
    class AddComponentCommand : public ICommand
    {
    public:
        AddComponentCommand(Scene& scene, uint64_t uuid, const std::string& label) : m_Scene(scene), m_UUID(uuid), m_Label(label)
        {

        }

        void Execute() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && !l_Entity.HasComponent<T>())
            {
                l_Entity.AddComponent<T>(m_Value);
            }
        }

        void Undo() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<T>())
            {
                l_Entity.RemoveComponent<T>();
            }
        }

        std::string GetName() const override { return "Add " + m_Label; }

    private:
        Scene& m_Scene;
        uint64_t m_UUID = 0;
        std::string m_Label;
        T m_Value{};
    };

    template<typename T>
    class RemoveComponentCommand : public ICommand
    {
    public:
        RemoveComponentCommand(Scene& scene, uint64_t uuid, const std::string& label) : m_Scene(scene), m_UUID(uuid), m_Label(label)
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<T>())
            {
                m_Value = l_Entity.GetComponent<T>();
            }
        }

        void Execute() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<T>())
            {
                l_Entity.RemoveComponent<T>();
            }
        }

        void Undo() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && !l_Entity.HasComponent<T>())
            {
                l_Entity.AddComponent<T>(m_Value);
            }
        }

        std::string GetName() const override { return "Remove " + m_Label; }

    private:
        Scene& m_Scene;
        uint64_t m_UUID = 0;
        std::string m_Label;
        T m_Value{};
    };
}