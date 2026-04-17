#include "Forge/Panels/ContentBrowserPanel.h"

#include <imgui.h>

#include <algorithm>
#include <vector>

namespace Forge
{
    ContentBrowserPanel::ContentBrowserPanel(std::string name)
        : Panel(std::move(name)),
          m_BaseDirectory("assets"),
          m_CurrentDirectory(m_BaseDirectory)
    {
    }

    void ContentBrowserPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        // Navigation bar
        const bool l_AtRoot = (m_CurrentDirectory == m_BaseDirectory);

        if (l_AtRoot)
            ImGui::BeginDisabled();

        if (ImGui::Button("<- Back"))
            m_CurrentDirectory = m_CurrentDirectory.parent_path();

        if (l_AtRoot)
            ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::TextUnformatted(m_CurrentDirectory.generic_string().c_str());

        ImGui::Separator();

        if (!std::filesystem::exists(m_CurrentDirectory) || !std::filesystem::is_directory(m_CurrentDirectory))
        {
            ImGui::TextDisabled("Directory not found: %s", m_CurrentDirectory.generic_string().c_str());
            ImGui::End();
            return;
        }

        // Collect and sort entries — directories first, then files
        std::vector<std::filesystem::directory_entry> l_Dirs, l_Files;

        for (const auto& l_Entry : std::filesystem::directory_iterator(m_CurrentDirectory))
        {
            if (l_Entry.is_directory())
                l_Dirs.push_back(l_Entry);
            else
                l_Files.push_back(l_Entry);
        }

        auto l_ByName = [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
        {
            return a.path().filename() < b.path().filename();
        };

        std::sort(l_Dirs.begin(),  l_Dirs.end(),  l_ByName);
        std::sort(l_Files.begin(), l_Files.end(), l_ByName);

        // Directories
        for (const auto& l_Dir : l_Dirs)
        {
            const std::string l_Label = "[DIR]  " + l_Dir.path().filename().string();
            if (ImGui::Selectable(l_Label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    m_CurrentDirectory = l_Dir.path();
            }
        }

        // Files
        for (const auto& l_File : l_Files)
        {
            const std::string l_Ext  = l_File.path().extension().string();
            const std::string l_Name = l_File.path().filename().string();

            // Simple type prefix for readability before thumbnail support (Phase 18 gap)
            const char* l_Prefix = "       ";
            if      (l_Ext == ".trinity") l_Prefix = "[SCN]  ";
            else if (l_Ext == ".fbx" || l_Ext == ".obj" || l_Ext == ".gltf" || l_Ext == ".glb") l_Prefix = "[MSH]  ";
            else if (l_Ext == ".png" || l_Ext == ".jpg" || l_Ext == ".jpeg" || l_Ext == ".tga") l_Prefix = "[TEX]  ";
            else if (l_Ext == ".wav" || l_Ext == ".ogg") l_Prefix = "[AUD]  ";
            else if (l_Ext == ".lua") l_Prefix = "[LUA]  ";
            else if (l_Ext == ".prefab") l_Prefix = "[PFB]  ";

            ImGui::Selectable((l_Prefix + l_Name).c_str());
        }

        ImGui::End();
    }
}
