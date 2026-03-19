#include "Forge/Panels/ContentBrowserPanel.h"

#include "Trinity/Assets/AssetRegistry.h"

#include <imgui.h>

void ContentBrowserPanel::SetRootPath(const std::filesystem::path& path)
{
    Trinity::AssetRegistry::Get().Scan(path);
}

void ContentBrowserPanel::Refresh()
{
    Trinity::AssetRegistry::Get().Refresh();
}

void ContentBrowserPanel::OnImGuiRender()
{
    ImGui::Begin("Content Browser");

    if (ImGui::Button("Refresh"))
    {
        Trinity::AssetRegistry::Get().Refresh();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputText("##Search", m_SearchBuffer, sizeof(m_SearchBuffer));

    const Trinity::AssetRegistry& l_Registry = Trinity::AssetRegistry::Get();
    if (l_Registry.IsScanned())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%zu asset(s)", l_Registry.GetAll().size());
    }

    ImGui::Separator();

    ImGui::BeginChild("##Categories", ImVec2(140.0f, 0.0f), true);
    DrawCategoryList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##AssetList", ImVec2(0.0f, 0.0f), false);
    DrawAssetList();
    ImGui::EndChild();

    ImGui::End();
}

void ContentBrowserPanel::DrawCategoryList()
{
    struct CategoryEntry
    {
        const char* m_Label;
        Trinity::AssetType m_Type;
    };

    static constexpr CategoryEntry k_Categories[] =
    {
        { "All",       Trinity::AssetType::Unknown  },
        { "Scenes",    Trinity::AssetType::Scene    },
        { "Meshes",    Trinity::AssetType::Mesh     },
        { "Textures",  Trinity::AssetType::Texture  },
        { "Materials", Trinity::AssetType::Material },
        { "Scripts",   Trinity::AssetType::Script   },
    };

    for (const CategoryEntry& it_Category : k_Categories)
    {
        const bool l_Selected = (m_TypeFilter == it_Category.m_Type);

        if (ImGui::Selectable(it_Category.m_Label, l_Selected))
        {
            m_TypeFilter = it_Category.m_Type;
        }
    }
}

void ContentBrowserPanel::DrawAssetList()
{
    const Trinity::AssetRegistry& l_Registry = Trinity::AssetRegistry::Get();

    if (!l_Registry.IsScanned())
    {
        ImGui::TextDisabled("No assets scanned.");
        ImGui::TextDisabled("Click Refresh to scan the Assets directory.");

        return;
    }

    const std::string l_Search(m_SearchBuffer);
    const auto& l_Assets = l_Registry.GetAll();

    if (l_Assets.empty())
    {
        ImGui::TextDisabled("No assets found in: %s", l_Registry.GetRootPath().string().c_str());
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 4.0f));

    if (ImGui::BeginTable("##AssetTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const Trinity::AssetMetadata& it_Asset : l_Assets)
        {
            if (m_TypeFilter != Trinity::AssetType::Unknown && it_Asset.m_Type != m_TypeFilter)
            {
                continue;
            }

            if (!l_Search.empty() && it_Asset.m_Name.find(l_Search) == std::string::npos)
            {
                continue;
            }

            ImGui::PushID(static_cast<int>(it_Asset.m_UUID));
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("%s", Trinity::AssetRegistry::TypeToIcon(it_Asset.m_Type));

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", it_Asset.m_Name.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("%s", it_Asset.m_FilePath.string().c_str());

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
}