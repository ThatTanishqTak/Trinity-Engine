#include <Forge/Editor/Panels/ContentBrowserPanel.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <system_error>
#include <vector>

#include <imgui.h>

#include <Forge/Editor/EditorIcons.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Renderer/Materials/Material.h>
#include <Trinity/Serialization/MaterialSerializer.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    static std::string ToLower(const std::string& text)
    {
        std::string l_Result = text;
        std::transform(l_Result.begin(), l_Result.end(), l_Result.begin(), [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

        return l_Result;
    }

    static const char* AssetIcon(AssetType type)
    {
        switch (type)
        {
            case AssetType::Mesh:
                return ICON_FA_CUBE;
            case AssetType::Texture:
                return ICON_FA_IMAGE;
            case AssetType::Material:
            case AssetType::MaterialInstance:
                return ICON_FA_PAINTBRUSH;
            case AssetType::Audio:
                return ICON_FA_VOLUME_HIGH;
            default:
                return ICON_FA_FILE;
        }
    }

    static const char* DragPayloadId(AssetType type)
    {
        if (type == AssetType::Mesh)
        {
            return "TRINITY_ASSET_MESH";
        }

        if (type == AssetType::Material || type == AssetType::MaterialInstance)
        {
            return "TRINITY_ASSET_MATERIAL";
        }

        return nullptr;
    }

    static bool HasSubdirectory(const std::filesystem::path& directory)
    {
        std::error_code l_Error;
        for (const std::filesystem::directory_entry& it_Entry : std::filesystem::directory_iterator(directory, l_Error))
        {
            if (it_Entry.is_directory())
            {
                return true;
            }
        }

        return false;
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");
        RenderContents();
        ImGui::End();
    }

    void ContentBrowserPanel::EnsureRoot()
    {
        if (m_Initialized)
        {
            return;
        }

        m_AssetsRoot = m_Engine.GetPlatform().GetFileSystem().Resolve(BaseDirectory::Executable, "Assets");
        m_CurrentDirectory = m_AssetsRoot;
        m_Initialized = true;
    }

    void ContentBrowserPanel::RenderContents()
    {
        if (!m_Engine.HasAssetDatabase())
        {
            return;
        }

        EnsureRoot();

        AssetDatabase& l_Assets = m_Engine.GetAssetDatabase();

        RenderToolbar(l_Assets);
        ImGui::Separator();

        float l_TreeWidth = ImGui::GetFontSize() * 12.0f;

        ImGui::BeginChild("##ContentBrowserTree", ImVec2(l_TreeWidth, 0.0f), ImGuiChildFlags_Borders);
        RenderFolderTree();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##ContentBrowserGrid", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        RenderGrid(l_Assets);
        ImGui::EndChild();
    }

    void ContentBrowserPanel::RenderToolbar(AssetDatabase& assetDatabase)
    {
        if (ImGui::Button(ICON_FA_ARROWS_ROTATE " Refresh"))
        {
            assetDatabase.Refresh();
            ReResolveModifiedMeshes();
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_PLUS " Material"))
        {
            std::error_code l_DirectoryError;
            std::filesystem::create_directories(m_CurrentDirectory, l_DirectoryError);

            std::filesystem::path l_File;
            uint32_t l_Index = 0;
            do
            {
                l_File = m_CurrentDirectory / ("Material_" + std::to_string(l_Index++) + ".material");
            } while (std::filesystem::exists(l_File));

            Material l_Material;
            if (MaterialSerializer::SerializeMaterial(l_Material, l_File))
            {
                assetDatabase.Refresh();
            }
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
        ImGui::InputTextWithHint("##ContentBrowserSearch", ICON_FA_SLIDERS "  Search...", m_SearchBuffer, sizeof(m_SearchBuffer));

        RenderBreadcrumbs();
    }

    void ContentBrowserPanel::RenderBreadcrumbs()
    {
        if (ImGui::SmallButton("Assets"))
        {
            m_CurrentDirectory = m_AssetsRoot;
        }

        std::error_code l_Error;
        std::filesystem::path l_Relative = std::filesystem::relative(m_CurrentDirectory, m_AssetsRoot, l_Error);
        if (l_Error || l_Relative.empty() || l_Relative == std::filesystem::path("."))
        {
            return;
        }

        std::filesystem::path l_Accumulated = m_AssetsRoot;
        for (const std::filesystem::path& it_Part : l_Relative)
        {
            l_Accumulated /= it_Part;

            ImGui::SameLine();
            ImGui::TextDisabled("/");
            ImGui::SameLine();

            ImGui::PushID(l_Accumulated.string().c_str());
            if (ImGui::SmallButton(it_Part.string().c_str()))
            {
                m_CurrentDirectory = l_Accumulated;
            }
            ImGui::PopID();
        }
    }

    void ContentBrowserPanel::RenderFolderTree()
    {
        ImGuiTreeNodeFlags l_RootFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (m_CurrentDirectory == m_AssetsRoot)
        {
            l_RootFlags |= ImGuiTreeNodeFlags_Selected;
        }

        bool l_Open = ImGui::TreeNodeEx("##AssetsRoot", l_RootFlags, ICON_FA_FOLDER "  Assets");
        if (ImGui::IsItemClicked())
        {
            m_CurrentDirectory = m_AssetsRoot;
        }

        if (l_Open)
        {
            RenderFolderNode(m_AssetsRoot);
            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::RenderFolderNode(const std::filesystem::path& directory)
    {
        std::error_code l_Error;
        for (const std::filesystem::directory_entry& it_Entry : std::filesystem::directory_iterator(directory, l_Error))
        {
            if (!it_Entry.is_directory())
            {
                continue;
            }

            const std::filesystem::path& l_Child = it_Entry.path();

            ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!HasSubdirectory(l_Child))
            {
                l_Flags |= ImGuiTreeNodeFlags_Leaf;
            }
            if (l_Child == m_CurrentDirectory)
            {
                l_Flags |= ImGuiTreeNodeFlags_Selected;
            }

            std::string l_Label = std::string(ICON_FA_FOLDER) + "  " + l_Child.filename().string();

            ImGui::PushID(l_Child.string().c_str());
            bool l_Open = ImGui::TreeNodeEx("##folder", l_Flags, "%s", l_Label.c_str());
            if (ImGui::IsItemClicked())
            {
                m_CurrentDirectory = l_Child;
            }

            if (l_Open)
            {
                RenderFolderNode(l_Child);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    void ContentBrowserPanel::RenderGrid(AssetDatabase& assetDatabase)
    {
        std::error_code l_Error;
        if (!std::filesystem::exists(m_CurrentDirectory, l_Error))
        {
            ImGui::TextDisabled("(folder not found)");

            return;
        }

        std::string l_Filter = ToLower(m_SearchBuffer);

        std::vector<std::filesystem::path> l_Directories;
        std::vector<std::filesystem::path> l_Files;

        for (const std::filesystem::directory_entry& it_Entry : std::filesystem::directory_iterator(m_CurrentDirectory, l_Error))
        {
            if (it_Entry.is_directory())
            {
                l_Directories.push_back(it_Entry.path());
            }
            else if (it_Entry.is_regular_file())
            {
                std::string l_Extension = it_Entry.path().extension().string();
                if (l_Extension == ".meta")
                {
                    continue;
                }

                if (AssetTypeFromExtension(l_Extension) == AssetType::None)
                {
                    continue;
                }

                l_Files.push_back(it_Entry.path());
            }
        }

        float l_Thumbnail = ImGui::GetFontSize() * 3.2f;
        float l_CellSize = l_Thumbnail + ImGui::GetStyle().ItemSpacing.x + 16.0f;
        float l_Available = ImGui::GetContentRegionAvail().x;
        int l_Columns = static_cast<int>(l_Available / l_CellSize);
        if (l_Columns < 1)
        {
            l_Columns = 1;
        }

        ImGui::Columns(l_Columns, "##ContentBrowserColumns", false);

        for (const std::filesystem::path& it_Directory : l_Directories)
        {
            std::string l_Name = it_Directory.filename().string();
            if (!l_Filter.empty() && ToLower(l_Name).find(l_Filter) == std::string::npos)
            {
                continue;
            }

            ImGui::PushID(it_Directory.string().c_str());

            ImGui::SetWindowFontScale(2.2f);
            ImGui::Button(ICON_FA_FOLDER, ImVec2(l_Thumbnail, l_Thumbnail));
            ImGui::SetWindowFontScale(1.0f);

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                m_CurrentDirectory = it_Directory;
            }

            ImGui::TextWrapped("%s", l_Name.c_str());

            ImGui::NextColumn();
            ImGui::PopID();
        }

        for (const std::filesystem::path& it_File : l_Files)
        {
            std::string l_Name = it_File.stem().string();
            if (!l_Filter.empty() && ToLower(l_Name).find(l_Filter) == std::string::npos)
            {
                continue;
            }

            AssetType l_Type = AssetTypeFromExtension(it_File.extension().string());

            ImGui::PushID(it_File.string().c_str());

            ImGui::SetWindowFontScale(2.2f);
            ImGui::Button(AssetIcon(l_Type), ImVec2(l_Thumbnail, l_Thumbnail));
            ImGui::SetWindowFontScale(1.0f);

            const char* l_Payload = DragPayloadId(l_Type);
            if (l_Payload != nullptr)
            {
                std::error_code l_RelError;
                std::filesystem::path l_Relative = std::filesystem::relative(it_File, m_AssetsRoot, l_RelError);
                std::string l_SourcePath = "Assets/" + l_Relative.generic_string();
                UUID l_Id = assetDatabase.GetAssetByPath(l_SourcePath);

                if (static_cast<uint64_t>(l_Id) != 0 && ImGui::BeginDragDropSource())
                {
                    uint64_t l_Raw = static_cast<uint64_t>(l_Id);
                    ImGui::SetDragDropPayload(l_Payload, &l_Raw, sizeof(l_Raw));
                    ImGui::TextUnformatted(l_Name.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            ImGui::TextWrapped("%s", l_Name.c_str());

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
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