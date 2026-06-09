#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Forge/Editor/ICommand.h>
#include <Forge/Editor/Commands/EntityCommands.h>

#include <Trinity/Core/UUID.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>

namespace Trinity
{
    class SetMaterialCommand : public ICommand
    {
    public:
        SetMaterialCommand(Scene& scene, uint64_t uuid, uint32_t slot, UUID newMaterial) : m_Scene(scene), m_UUID(uuid), m_Slot(slot), m_NewMaterial(newMaterial)
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (l_Entity.IsValid() && l_Entity.HasComponent<MeshRendererComponent>())
            {
                m_OldMaterials = l_Entity.GetComponent<MeshRendererComponent>().Materials;
            }
        }

        void Execute() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (!l_Entity.IsValid() || !l_Entity.HasComponent<MeshRendererComponent>())
            {
                return;
            }

            std::vector<UUID>& l_Materials = l_Entity.GetComponent<MeshRendererComponent>().Materials;
            l_Materials = m_OldMaterials;
            if (m_Slot >= l_Materials.size())
            {
                l_Materials.resize(m_Slot + 1, UUID(0));
            }

            l_Materials[m_Slot] = m_NewMaterial;
        }

        void Undo() override
        {
            Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
            if (!l_Entity.IsValid() || !l_Entity.HasComponent<MeshRendererComponent>())
            {
                return;
            }

            l_Entity.GetComponent<MeshRendererComponent>().Materials = m_OldMaterials;
        }

        std::string GetName() const override { return "Set Material"; }

    private:
        Scene& m_Scene;
        uint64_t m_UUID = 0;
        uint32_t m_Slot = 0;
        UUID m_NewMaterial = UUID(0);
        std::vector<UUID> m_OldMaterials;
    };
}