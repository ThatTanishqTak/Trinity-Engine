#include "Forge/Panels/ContentBrowserPanel.h"
#include "Forge/AssetPayload.h"

#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Renderer/Renderer.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
    Trinity::AssetType AssetTypeFromFileIconType(Forge::FileIconType type)
    {
        switch (type)
        {
            case Forge::FileIconType::Mesh:
            {
                return Trinity::AssetType::Mesh;
            }
            case Forge::FileIconType::Texture:
            {
                return Trinity::AssetType::Texture;
            }
            case Forge::FileIconType::Scene:
            {
                return Trinity::AssetType::Scene;
            }
            default:
            {
                return Trinity::AssetType::None;
            }
        }
    }
}

namespace Forge
{
    std::vector<uint8_t> ContentBrowserPanel::MakeIconPixels(uint8_t r, uint8_t g, uint8_t b)
    {
        constexpr int l_Size = 32;
        constexpr int l_Border = 2;

        std::vector<uint8_t> l_Pixels(l_Size * l_Size * 4);

        for (int y = 0; y < l_Size; ++y)
        {
            for (int x = 0; x < l_Size; ++x)
            {
                const int l_Index  = (y * l_Size + x) * 4;
                const bool l_IsBorder = (x < l_Border || x >= l_Size - l_Border || y < l_Border || y >= l_Size - l_Border);

                if (l_IsBorder)
                {
                    // Slightly brightened border
                    l_Pixels[l_Index + 0] = static_cast<uint8_t>(std::min(255, static_cast<int>(r) + 70));
                    l_Pixels[l_Index + 1] = static_cast<uint8_t>(std::min(255, static_cast<int>(g) + 70));
                    l_Pixels[l_Index + 2] = static_cast<uint8_t>(std::min(255, static_cast<int>(b) + 70));
                }
                else
                {
                    l_Pixels[l_Index + 0] = r;
                    l_Pixels[l_Index + 1] = g;
                    l_Pixels[l_Index + 2] = b;
                }

                l_Pixels[l_Index + 3] = 255;
            }
        }

        return l_Pixels;
    }

    std::vector<uint8_t> ContentBrowserPanel::MakeFolderPixels(uint8_t r, uint8_t g, uint8_t b)
    {
        constexpr int l_Size = 32;

        std::vector<uint8_t> l_Pixels(l_Size * l_Size * 4, 0);

        // Colour helpers
        auto a_Fill = [&](int x, int y, uint8_t pr, uint8_t pg, uint8_t pb)
        {
            if (x < 0 || x >= l_Size || y < 0 || y >= l_Size)
            {
                return;
            }
            
            const int l_Index = (y * l_Size + x) * 4;
            l_Pixels[l_Index + 0] = pr;
            l_Pixels[l_Index + 1] = pg;
            l_Pixels[l_Index + 2] = pb;
            l_Pixels[l_Index + 3] = 255;
        };

        const uint8_t l_BackgroundR = static_cast<uint8_t>(std::min(255, static_cast<int>(r) + 70));
        const uint8_t l_BackgroundG = static_cast<uint8_t>(std::min(255, static_cast<int>(g) + 70));
        const uint8_t l_BackgroundB = static_cast<uint8_t>(std::min(255, static_cast<int>(b) + 70));

        for (int y = 2; y < 8; ++y)
        {
            for (int x = 2; x < 16; ++x)
            {
                a_Fill(x, y, (x < 3 || x > 14 || y < 3 || y > 6) ? l_BackgroundR : r, (x < 3 || x > 14 || y < 3 || y > 6) ? l_BackgroundG : g, (x < 3 || x > 14 || y < 3 || y > 6) ? l_BackgroundB : b);
            }
        }

        // Body (rows 6-29, cols 2-29)
        for (int y = 6; y < 30; ++y)
        {
            for (int x = 2; x < 30; ++x)
            {
                const bool l_Edge = (x == 2 || x == 29 || y == 6 || y == 29);
                a_Fill(x, y, l_Edge ? l_BackgroundR : r, l_Edge ? l_BackgroundG : g, l_Edge ? l_BackgroundB : b);
            }
        }

        return l_Pixels;
    }

    ContentBrowserPanel::ContentBrowserPanel(std::string name) : Panel(std::move(name)), m_BaseDirectory("assets"), m_CurrentDirectory(m_BaseDirectory)
    {

    }

    void ContentBrowserPanel::OnInitialize()
    {
        LoadIcons();
    }

    void ContentBrowserPanel::OnShutdown()
    {
        for (auto& it_Icon : m_Icons)
        {
            if (it_Icon.ImGuiID != 0)
            {
                Trinity::ImGuiLayer::Get().UnregisterTexture(it_Icon.ImGuiID);
                it_Icon.ImGuiID = 0;
            }
        }
    }

    void ContentBrowserPanel::LoadIcons()
    {
        struct IconDefincation
        {
            FileIconType Type;
            
            uint8_t R;
            uint8_t G;
            uint8_t B;

            const char* Label;
            const char* Path;
        };

        const IconDefincation l_Definations[] =
        {
            { FileIconType::Folder, 212, 175, 55, "DIR", "assets/resources/icons/folder.png" },
            { FileIconType::Scene, 76, 175, 80, "SCN", "assets/resources/icons/scene.png" },
            { FileIconType::Mesh, 33, 150, 243, "MSH", "assets/resources/icons/mesh.png" },
            { FileIconType::Texture, 233, 30, 99, "TEX", "assets/resources/icons/Texture.png" },
            { FileIconType::Audio, 255, 87, 34, "AUD", "assets/resources/icons/audio.png" },
            { FileIconType::Script, 156, 39, 176, "LUA", "assets/resources/icons/script.png" },
            { FileIconType::Prefab, 0, 188, 212, "PFB", "assets/resources/icons/prefab.png" },
            { FileIconType::Generic, 158, 158, 158, "FILE", "assets/assets/resources/icons/file.png" },
        };

        for (const auto& it_Defination : l_Definations)
        {
            const size_t l_Index = static_cast<size_t>(it_Defination.Type);
            FileIcon& l_Icon = m_Icons[l_Index];

            l_Icon.Label = it_Defination.Label;
            l_Icon.Color = IM_COL32(it_Defination.R, it_Defination.G, it_Defination.B, 255);

            // Try loading from disk first
            std::shared_ptr<Trinity::Texture> l_Texture = Trinity::Renderer::LoadTextureFromFile(it_Defination.Path);

            if (!l_Texture)
            {
                // Fall back to procedurally generated Texture
                std::vector<uint8_t> l_Pixels = (it_Defination.Type == FileIconType::Folder) ? MakeFolderPixels(it_Defination.R, it_Defination.G, it_Defination.B) : MakeIconPixels(it_Defination.R, it_Defination.G, it_Defination.B);

                l_Texture = Trinity::Renderer::CreateTextureFromData(l_Pixels.data(), 32, 32);
            }

            if (l_Texture)
            {
                l_Icon.Texture = l_Texture;
                l_Icon.ImGuiID = Trinity::ImGuiLayer::Get().RegisterTexture(l_Texture);
            }
        }

        m_IconsLoaded = true;
    }

    FileIconType ContentBrowserPanel::FileTypeFromExtension(const std::string& extension) const
    {
        if (extension == ".tscene")
        {
            return FileIconType::Scene;
        }

        if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".glb")
        {
            return FileIconType::Mesh;
        }

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga")
        {
            return FileIconType::Texture;
        }

        if (extension == ".wav" || extension == ".ogg")
        {
            return FileIconType::Audio;
        }

        if (extension == ".lua")
        {
            return FileIconType::Script;
        }

        if (extension == ".prefab")
        {
            return FileIconType::Prefab;
        }

        return FileIconType::Generic;
    }

    void ContentBrowserPanel::RenderEntry(const std::string& name, FileIconType iconType, bool /*isDirectory*/, bool& outDoubleClicked)
    {
        const FileIcon& l_Icon = m_Icons[static_cast<size_t>(iconType)];

        ImGui::PushID(name.c_str());

        const ImVec2 l_CursorPosition = ImGui::GetCursorScreenPos();
        const bool l_Activated = ImGui::Selectable("##entry", false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, s_RowHeight));

        outDoubleClicked = l_Activated && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        ImDrawList* l_DrawList = ImGui::GetWindowDrawList();

        const float l_IconY = l_CursorPosition.y + (s_RowHeight - s_IconSize) * 0.5f;
        const ImVec2 l_IconMin = ImVec2(l_CursorPosition.x + s_IconPadding, l_IconY);
        const ImVec2 l_IconMax = ImVec2(l_IconMin.x + s_IconSize, l_IconMin.y + s_IconSize);

        if (l_Icon.ImGuiID != 0)
        {
            l_DrawList->AddImage(static_cast<ImTextureID>(l_Icon.ImGuiID), l_IconMin, l_IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        }
        else
        {
            l_DrawList->AddRectFilled(l_IconMin, l_IconMax, l_Icon.Color, 3.0f);

            const ImVec2 ts = ImGui::CalcTextSize(l_Icon.Label);
            l_DrawList->AddText(ImVec2(l_IconMin.x + (s_IconSize - ts.x) * 0.5f, l_IconMin.y + (s_IconSize - ts.y) * 0.5f), IM_COL32(255, 255, 255, 255), l_Icon.Label);
        }

        const float l_TextX = l_CursorPosition.x + s_IconPadding + s_IconSize + 6.0f;
        const float l_TextY = l_CursorPosition.y + (s_RowHeight - ImGui::GetTextLineHeight()) * 0.5f;

        l_DrawList->AddText(ImVec2(l_TextX, l_TextY), ImGui::GetColorU32(ImGuiCol_Text), name.c_str());

        ImGui::PopID();
    }

    void ContentBrowserPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        if (!m_IconsLoaded)
        {
            LoadIcons();
        }

        // Navigation bar
        const bool l_AtRoot = (m_CurrentDirectory == m_BaseDirectory);

        if (l_AtRoot)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("<- Back"))
        {
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        }

        if (l_AtRoot)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(m_CurrentDirectory.generic_string().c_str());

        ImGui::Separator();

        if (!std::filesystem::exists(m_CurrentDirectory) || !std::filesystem::is_directory(m_CurrentDirectory))
        {
            ImGui::TextDisabled("Directory not found: %s", m_CurrentDirectory.generic_string().c_str());
            ImGui::End();

            return;
        }

        std::vector<std::filesystem::directory_entry> l_Directories;
        std::vector<std::filesystem::directory_entry> l_Files;

        for (const auto& it_Entry : std::filesystem::directory_iterator(m_CurrentDirectory))
        {
            if (it_Entry.is_directory())
            {
                l_Directories.push_back(it_Entry);
            }
            else
            {
                l_Files.push_back(it_Entry);
            }
        }

        auto a_ByName = [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
        {
            return a.path().filename() < b.path().filename();
        };

        std::sort(l_Directories.begin(),  l_Directories.end(),  a_ByName);
        std::sort(l_Files.begin(), l_Files.end(), a_ByName);

        // Directories
        for (const auto& it_Directory : l_Directories)
        {
            const std::string l_Name = it_Directory.path().filename().string();
            bool l_DoubleClicked = false;

            RenderEntry(l_Name, FileIconType::Folder, true, l_DoubleClicked);

            if (l_DoubleClicked)
            {
                m_CurrentDirectory = it_Directory.path();
            }
        }

        // Files
        for (const auto& it_File : l_Files)
        {
            const std::string l_Extension = it_File.path().extension().string();

            if (l_Extension == ".meta")
            {
                continue;
            }

            const std::string l_Name = it_File.path().filename().string();
            const FileIconType l_Type = FileTypeFromExtension(l_Extension);
            bool l_DoubleClicked = false;

            RenderEntry(l_Name, l_Type, false, l_DoubleClicked);

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                AssetPayload l_Payload{};
                l_Payload.Type = AssetTypeFromFileIconType(l_Type);

                const std::string l_FullPath = it_File.path().string();
                std::strncpy(l_Payload.Path, l_FullPath.c_str(), sizeof(l_Payload.Path) - 1);

                ImGui::SetDragDropPayload(AssetPayloadID, &l_Payload, sizeof(l_Payload));

                const FileIcon& l_Icon = m_Icons[static_cast<size_t>(l_Type)];
                if (l_Icon.ImGuiID != 0)
                {
                    ImGui::Image(static_cast<ImTextureID>(l_Icon.ImGuiID), ImVec2(16.0f, 16.0f));
                }
                else
                {
                    const ImVec4 l_Color = ImGui::ColorConvertU32ToFloat4(l_Icon.Color);
                    ImGui::ColorButton("##icon", l_Color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(16.0f, 16.0f));
                }

                ImGui::SameLine(0.0f, 6.0f);
                ImGui::Text("%s  %s", l_Icon.Label, l_Name.c_str());

                ImGui::EndDragDropSource();
            }
        }

        ImGui::End();
    }
}