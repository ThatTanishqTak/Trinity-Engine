#pragma once

#include <cstdint>
#include <string>

#include <Forge/Editor/ICommand.h>
#include <Forge/Editor/Commands/EntityCommands.h>

#include <Trinity/Core/UUID.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    class SetMeshCommand : public ICommand
    {
    public:
        SetMeshCommand(Scene& scene, AssetDatabase& assetDatabase, uint64_t uuid, UUID newMeshAsset) : m_Scene(scene), m_AssetDatabase(assetDatabase), m_UUID(uuid), m_NewMeshAsset(newMeshAsset)
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<MeshRendererComponent>())
            {
                m_OldMeshAsset = l_Entity.GetComponent<MeshRendererComponent>().MeshAsset;
            }
        }

        void Execute() override
        {
            Apply(m_NewMeshAsset);
        }

        void Undo() override
        {
            Apply(m_OldMeshAsset);
        }

        std::string GetName() const override { return "Set Mesh"; }

    private:
        void Apply(UUID asset)
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (!l_Entity.IsValid() || !l_Entity.HasComponent<MeshRendererComponent>())
            {
                return;
            }

            MeshRendererComponent& l_Component = l_Entity.GetComponent<MeshRendererComponent>();
            l_Component.MeshAsset = asset;
            l_Component.MeshReference = m_AssetDatabase.ResolveMesh(asset);
        }

        Scene& m_Scene;
        AssetDatabase& m_AssetDatabase;
        uint64_t m_UUID = 0;
        UUID m_NewMeshAsset = UUID(0);
        UUID m_OldMeshAsset = UUID(0);
    };
}