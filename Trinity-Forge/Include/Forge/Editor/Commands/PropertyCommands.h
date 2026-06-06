#pragma once

#include <cstdint>
#include <string>

#include <Forge/Editor/ICommand.h>
#include <Forge/Editor/Commands/EntityCommands.h>

#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>

namespace Trinity
{
    class SetTransformCommand : public ICommand
    {
    public:
        SetTransformCommand(Scene& scene, uint64_t uuid, const TransformComponent& oldTransform, const TransformComponent& newTransform) : m_Scene(scene), m_UUID(uuid), m_Old(oldTransform), m_New(newTransform)
        {

        }

        void Execute() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<TransformComponent>())
            {
                l_Entity.GetComponent<TransformComponent>() = m_New;
            }
        }

        void Undo() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<TransformComponent>())
            {
                l_Entity.GetComponent<TransformComponent>() = m_Old;
            }
        }

        std::string GetName() const override { return "Edit Transform"; }

    private:
        Scene& m_Scene;
        uint64_t m_UUID = 0;
        TransformComponent m_Old;
        TransformComponent m_New;
    };

    class SetCameraPrimaryCommand : public ICommand
    {
    public:
        SetCameraPrimaryCommand(Scene& scene, uint64_t uuid, bool oldValue, bool newValue) : m_Scene(scene), m_UUID(uuid), m_Old(oldValue), m_New(newValue)
        {

        }

        void Execute() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<CameraComponent>())
            {
                l_Entity.GetComponent<CameraComponent>().Primary = m_New;
            }
        }

        void Undo() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<CameraComponent>())
            {
                l_Entity.GetComponent<CameraComponent>().Primary = m_Old;
            }
        }

        std::string GetName() const override { return "Edit Camera"; }

    private:
        Scene& m_Scene;
        uint64_t m_UUID = 0;
        bool m_Old = false;
        bool m_New = false;
    };
}