#include <Forge/Editor/Panels/ContentBrowserPanel.h>

#include <vector>

#include <imgui.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");

        if (!m_Engine.HasAssetDatabase())
        {
            ImGui::End();

            return;
        }

        AssetDatabase& l_Assets = m_Engine.GetAssetDatabase();

        if (ImGui::Button("Refresh"))
        {
            l_Assets.Refresh();
            ReResolveModifiedMeshes();
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Meshes");

        for (UUID it_Asset : l_Assets.GetAssetsOfType(AssetType::Mesh))
        {
            const AssetMetadata* l_Meta = l_Assets.GetMetadata(it_Asset);
            if (l_Meta == nullptr)
            {
                continue;
            }

            ImGui::Selectable(l_Meta->SourcePath.c_str());

            if (ImGui::BeginDragDropSource())
            {
                uint64_t l_Payload = static_cast<uint64_t>(it_Asset);
                ImGui::SetDragDropPayload("TRINITY_ASSET_MESH", &l_Payload, sizeof(l_Payload));
                ImGui::TextUnformatted(l_Meta->SourcePath.c_str());
                ImGui::EndDragDropSource();
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Textures");

        for (UUID it_Asset : l_Assets.GetAssetsOfType(AssetType::Texture))
        {
            const AssetMetadata* l_Meta = l_Assets.GetMetadata(it_Asset);
            if (l_Meta != nullptr)
            {
                ImGui::BulletText("%s", l_Meta->SourcePath.c_str());
            }
        }

        ImGui::End();
    }

    void ContentBrowserPanel::ReResolveModifiedMeshes()
    {
        if (!m_Engine.HasScene() || !m_Engine.HasAssetDatabase())
        {
            return;
        }

        AssetDatabase& l_Assets = m_Engine.GetAssetDatabase();
        const std::vector<UUID>& l_Modified = l_Assets.GetModified();
        if (l_Modified.empty())
        {
            return;
        }

        Scene& l_Scene = m_Engine.GetScene();
        auto l_View = l_Scene.GetRegistry().view<MeshRendererComponent>();
        for (entt::entity it_Entity : l_View)
        {
            MeshRendererComponent& l_Component = l_View.get<MeshRendererComponent>(it_Entity);
            for (UUID it_Modified : l_Modified)
            {
                if (static_cast<uint64_t>(l_Component.MeshAsset) == static_cast<uint64_t>(it_Modified))
                {
                    l_Component.MeshReference = l_Assets.ResolveMesh(l_Component.MeshAsset);

                    break;
                }
            }
        }
    }
}