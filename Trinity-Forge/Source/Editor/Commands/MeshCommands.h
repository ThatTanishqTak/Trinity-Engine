#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <Editor/ICommand.h>
#include <Editor/Commands/EntityCommands.h>

#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Renderer/Meshes/MeshLibrary.h>

namespace Trinity
{
    class Mesh;

    class SetMeshCommand : public ICommand
    {
    public:
        SetMeshCommand(Scene& scene, MeshLibrary& meshLibrary, uint64_t uuid, const std::string& newPath) : m_Scene(scene), m_MeshLibrary(meshLibrary), m_UUID(uuid), m_NewPath(newPath)
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<MeshRendererComponent>())
            {
                MeshRendererComponent& l_Component = l_Entity.GetComponent<MeshRendererComponent>();
                m_OldPath = l_Component.MeshPath;
                m_OldMesh = l_Component.MeshReference;
            }
        }

        void Execute() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (!l_Entity.IsValid() || !l_Entity.HasComponent<MeshRendererComponent>())
            {
                return;
            }

            if (!m_NewMesh)
            {
                m_NewMesh = m_NewPath.empty() ? m_MeshLibrary.GetCube() : m_MeshLibrary.Load(m_NewPath);
            }

            MeshRendererComponent& l_Component = l_Entity.GetComponent<MeshRendererComponent>();
            l_Component.MeshPath = m_NewPath;
            l_Component.MeshReference = m_NewMesh;
        }

        void Undo() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (!l_Entity.IsValid() || !l_Entity.HasComponent<MeshRendererComponent>())
            {
                return;
            }

            MeshRendererComponent& l_Component = l_Entity.GetComponent<MeshRendererComponent>();
            l_Component.MeshPath = m_OldPath;
            l_Component.MeshReference = m_OldMesh;
        }

        std::string GetName() const override { return "Set Mesh"; }

    private:
        Scene& m_Scene;
        MeshLibrary& m_MeshLibrary;
        uint64_t m_UUID = 0;
        std::string m_NewPath;
        std::string m_OldPath;
        std::shared_ptr<Mesh> m_OldMesh;
        std::shared_ptr<Mesh> m_NewMesh;
    };
}