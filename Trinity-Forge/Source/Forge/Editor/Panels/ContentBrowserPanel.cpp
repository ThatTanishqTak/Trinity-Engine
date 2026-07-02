#include <Forge/Editor/Panels/ContentBrowserPanel.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
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

    // Generic payload carrying a filesystem path string; lets any tile (including folders and non-asset files) be dragged onto a folder to move it. Coexists with the typed asset payloads
    static const char* const k_ContentPathPayload = "TRINITY_CONTENT_PATH";

    // First free "Name_N" slot in the target folder, for the conflict modal's Keep Both option
    static std::filesystem::path UniqueDestination(const std::filesystem::path& targetDirectory, const std::filesystem::path& filename)
    {
        std::string l_Stem = filename.stem().string();
        std::string l_Extension = filename.extension().string();

        uint32_t l_Index = 1;
        std::filesystem::path l_Candidate;
        std::error_code l_Error;
        do
        {
            l_Candidate = targetDirectory / (l_Stem + "_" + std::to_string(l_Index++) + l_Extension);
        } while (std::filesystem::exists(l_Candidate, l_Error));

        return l_Candidate;
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

    static ImU32 AssetTypeColor(AssetType type)
    {
        switch (type)
        {
        case AssetType::Mesh:
            return IM_COL32(54, 170, 235, 255);   // blue
        case AssetType::Texture:
            return IM_COL32(216, 92, 92, 255);     // red
        case AssetType::Material:
            return IM_COL32(230, 168, 50, 255);    // amber
        case AssetType::MaterialInstance:
            return IM_COL32(230, 120, 40, 255);    // orange
        case AssetType::Audio:
            return IM_COL32(150, 110, 215, 255);   // purple
        default:
            return IM_COL32(130, 130, 130, 255);   // gray
        }
    }

    static void DrawTileDecoration(const ImVec2& thumbnailMin, float thumbnail, ImU32 typeColor, bool selected)
    {
        ImDrawList* l_DrawList = ImGui::GetWindowDrawList();

        // Unreal-style colored type strip along the bottom edge of the thumbnail.
        const float l_BarHeight = 3.0f;
        l_DrawList->AddRectFilled(ImVec2(thumbnailMin.x, thumbnailMin.y + thumbnail - l_BarHeight), ImVec2(thumbnailMin.x + thumbnail, thumbnailMin.y + thumbnail), typeColor);

        if (selected)
        {
            l_DrawList->AddRect(ImVec2(thumbnailMin.x - 1.0f, thumbnailMin.y - 1.0f), ImVec2(thumbnailMin.x + thumbnail + 1.0f, thumbnailMin.y + thumbnail + 1.0f), IM_COL32(66, 150, 250, 255), 3.0f, 0, 2.0f);
        }
    }

    static void CenteredClippedText(const std::string& text, float width)
    {
        ImVec2 l_Size = ImGui::CalcTextSize(text.c_str());
        if (l_Size.x <= width)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - l_Size.x) * 0.5f);
            ImGui::TextUnformatted(text.c_str());

            return;
        }

        std::string l_Clipped = text;
        while (l_Clipped.size() > 1 && ImGui::CalcTextSize((l_Clipped + "...").c_str()).x > width)
        {
            l_Clipped.pop_back();
        }

        l_Clipped += "...";
        ImGui::TextUnformatted(l_Clipped.c_str());
    }

    static void OpenInFileExplorer(const std::filesystem::path& target)
    {
        std::string l_Path = target.string();

#if defined(_WIN32)
        std::string l_Command = "explorer.exe /select,\"" + l_Path + "\"";
#elif defined(__APPLE__)
        std::string l_Command = "open -R \"" + l_Path + "\"";
#else
        std::string l_Command = "xdg-open \"" + target.parent_path().string() + "\"";
#endif

        std::system(l_Command.c_str());
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

        float l_FooterHeight = ImGui::GetFrameHeightWithSpacing();
        float l_TreeWidth = ImGui::GetFontSize() * 12.0f;

        ImGui::BeginChild("##ContentBrowserTree", ImVec2(l_TreeWidth, -l_FooterHeight), ImGuiChildFlags_Borders);
        RenderFolderTree(l_Assets);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##ContentBrowserGrid", ImVec2(0.0f, -l_FooterHeight), ImGuiChildFlags_Borders);
        RenderGrid(l_Assets);
        ImGui::EndChild();

        RenderMoveConflictModal(l_Assets);

        RenderFooter();
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

    void ContentBrowserPanel::RenderFolderTree(AssetDatabase& assetDatabase)
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

        AcceptMoveInto(m_AssetsRoot, assetDatabase);

        if (l_Open)
        {
            RenderFolderNode(m_AssetsRoot, assetDatabase);
            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::RenderFolderNode(const std::filesystem::path& directory, AssetDatabase& assetDatabase)
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

            AcceptMoveInto(l_Child, assetDatabase);

            if (l_Open)
            {
                RenderFolderNode(l_Child, assetDatabase);
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

        float l_Thumbnail = ImGui::GetFontSize() * 3.2f * m_ThumbnailScale;
        float l_CellSize = l_Thumbnail + ImGui::GetStyle().ItemSpacing.x + 16.0f;
        float l_Available = ImGui::GetContentRegionAvail().x;
        int l_Columns = static_cast<int>(l_Available / l_CellSize);
        if (l_Columns < 1)
        {
            l_Columns = 1;
        }

        m_AssetCount = 0;

        ImGui::Columns(l_Columns, "##ContentBrowserColumns", false);

        for (const std::filesystem::path& it_Directory : l_Directories)
        {
            std::string l_Name = it_Directory.filename().string();
            if (!l_Filter.empty() && ToLower(l_Name).find(l_Filter) == std::string::npos)
            {
                continue;
            }

            ImGui::PushID(it_Directory.string().c_str());

            bool l_Selected = m_SelectedAsset == it_Directory;
            ImVec2 l_ThumbMin = ImGui::GetCursorScreenPos();

            ImGui::PushStyleColor(ImGuiCol_Button, l_Selected ? ImVec4(0.18f, 0.30f, 0.46f, 1.0f) : ImVec4(0.105f, 0.105f, 0.105f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.200f, 0.200f, 0.200f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.160f, 0.160f, 0.160f, 1.0f));
            ImGui::SetWindowFontScale(2.2f * m_ThumbnailScale);
            ImGui::Button(ICON_FA_FOLDER, ImVec2(l_Thumbnail, l_Thumbnail));
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor(3);

            if (ImGui::IsItemClicked())
            {
                m_SelectedAsset = it_Directory;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                m_CurrentDirectory = it_Directory;
                m_SelectedAsset.clear();
            }

            if (ImGui::BeginDragDropSource())
            {
                std::string l_PathString = it_Directory.string();
                ImGui::SetDragDropPayload(k_ContentPathPayload, l_PathString.c_str(), l_PathString.size() + 1);
                ImGui::TextUnformatted((std::string(ICON_FA_FOLDER) + "  " + l_Name).c_str());
                ImGui::EndDragDropSource();
            }

            AcceptMoveInto(it_Directory, assetDatabase);

            RenderItemContextMenu(it_Directory, assetDatabase);

            DrawTileDecoration(l_ThumbMin, l_Thumbnail, IM_COL32(220, 180, 70, 255), l_Selected);
            RenderTileLabel(it_Directory, l_Name, l_Thumbnail, assetDatabase);

            ++m_AssetCount;

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

            bool l_Selected = m_SelectedAsset == it_File;
            ImVec2 l_ThumbMin = ImGui::GetCursorScreenPos();

            ImGui::PushStyleColor(ImGuiCol_Button, l_Selected ? ImVec4(0.18f, 0.30f, 0.46f, 1.0f) : ImVec4(0.105f, 0.105f, 0.105f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.200f, 0.200f, 0.200f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.160f, 0.160f, 0.160f, 1.0f));
            ImGui::SetWindowFontScale(2.2f * m_ThumbnailScale);
            ImGui::Button(AssetIcon(l_Type), ImVec2(l_Thumbnail, l_Thumbnail));
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor(3);

            if (ImGui::IsItemClicked())
            {
                m_SelectedAsset = it_File;
            }

            RenderItemContextMenu(it_File, assetDatabase);

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
            else if (ImGui::BeginDragDropSource())
            {
                // Non-asset file: carry its path so it can be moved between folders.
                std::string l_PathString = it_File.string();
                ImGui::SetDragDropPayload(k_ContentPathPayload, l_PathString.c_str(), l_PathString.size() + 1);
                ImGui::TextUnformatted(l_Name.c_str());
                ImGui::EndDragDropSource();
            }

            DrawTileDecoration(l_ThumbMin, l_Thumbnail, AssetTypeColor(l_Type), l_Selected);
            RenderTileLabel(it_File, l_Name, l_Thumbnail, assetDatabase);

            ++m_AssetCount;

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
        {
            m_SelectedAsset.clear();
        }

        if (ImGui::BeginPopupContextWindow("##GridContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem(ICON_FA_FOLDER "  New Folder"))
            {
                CreateNewFolder();
            }

            if (ImGui::MenuItem(ICON_FA_PLUS "  Import..."))
            {
                ImportAssets();
            }

            ImGui::EndPopup();
        }

        RenderDeleteModal(assetDatabase);
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

    void ContentBrowserPanel::MoveEntry(const std::filesystem::path& source, const std::filesystem::path& targetDirectory, AssetDatabase& assetDatabase)
    {
        std::error_code l_Error;

        // Already living in the target folder: nothing to do.
        if (std::filesystem::equivalent(source.parent_path(), targetDirectory, l_Error))
        {
            return;
        }

        // Never move a folder into itself or one of its own descendants.
        if (std::filesystem::is_directory(source, l_Error))
        {
            std::filesystem::path l_Probe = targetDirectory;
            while (!l_Probe.empty())
            {
                std::error_code l_SameError;
                if (std::filesystem::equivalent(l_Probe, source, l_SameError))
                {
                    return;
                }

                if (l_Probe == l_Probe.parent_path())
                {
                    break;
                }

                l_Probe = l_Probe.parent_path();
            }
        }

        std::filesystem::path l_Destination = targetDirectory / source.filename();

        std::error_code l_ExistsError;
        if (std::filesystem::exists(l_Destination, l_ExistsError))
        {
            // Name collision: stash the move and let the user decide via the conflict modal.
            m_PendingMoveSource = source;
            m_PendingMoveTarget = targetDirectory;
            m_OpenMoveConflictModal = true;

            return;
        }

        PerformMove(source, l_Destination, assetDatabase);
    }

    void ContentBrowserPanel::PerformMove(const std::filesystem::path& source, const std::filesystem::path& destination, AssetDatabase& assetDatabase)
    {
        std::error_code l_MoveError;
        std::filesystem::rename(source, destination, l_MoveError);
        if (l_MoveError)
        {
            return;
        }

        // Keep the .meta sidecar next to the file (directories don't have one).
        std::filesystem::path l_Meta = source;
        l_Meta += ".meta";
        std::error_code l_MetaExists;
        if (std::filesystem::exists(l_Meta, l_MetaExists))
        {
            std::filesystem::path l_NewMeta = destination;
            l_NewMeta += ".meta";
            std::error_code l_MetaError;
            std::filesystem::rename(l_Meta, l_NewMeta, l_MetaError);
        }

        if (m_SelectedAsset == source)
        {
            m_SelectedAsset = destination;
        }

        if (m_CurrentDirectory == source)
        {
            m_CurrentDirectory = destination;
        }

        assetDatabase.Refresh();
        ReResolveModifiedMeshes();
    }

    void ContentBrowserPanel::RenderMoveConflictModal(AssetDatabase& assetDatabase)
    {
        if (m_OpenMoveConflictModal)
        {
            ImGui::OpenPopup("Move Conflict");
            m_OpenMoveConflictModal = false;
        }

        ImVec2 l_Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(l_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Move Conflict", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("'%s' already exists in '%s'.", m_PendingMoveSource.filename().string().c_str(), m_PendingMoveTarget.filename().string().c_str());
            ImGui::TextDisabled("Replace deletes the existing entry. This cannot be undone.");
            ImGui::Separator();

            std::filesystem::path l_Destination = m_PendingMoveTarget / m_PendingMoveSource.filename();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.24f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.30f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.20f, 0.20f, 1.0f));
            if (ImGui::Button("Replace", ImVec2(120.0f, 0.0f)))
            {
                std::error_code l_RemoveError;
                std::filesystem::remove_all(l_Destination, l_RemoveError);

                std::filesystem::path l_Meta = l_Destination;
                l_Meta += ".meta";
                std::error_code l_MetaError;
                std::filesystem::remove(l_Meta, l_MetaError);

                if (!l_RemoveError)
                {
                    if (m_SelectedAsset == l_Destination)
                    {
                        m_SelectedAsset.clear();
                    }

                    PerformMove(m_PendingMoveSource, l_Destination, assetDatabase);
                }

                m_PendingMoveSource.clear();
                m_PendingMoveTarget.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::Button("Keep Both", ImVec2(120.0f, 0.0f)))
            {
                PerformMove(m_PendingMoveSource, UniqueDestination(m_PendingMoveTarget, m_PendingMoveSource.filename()), assetDatabase);

                m_PendingMoveSource.clear();
                m_PendingMoveTarget.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                m_PendingMoveSource.clear();
                m_PendingMoveTarget.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void ContentBrowserPanel::AcceptMoveInto(const std::filesystem::path& targetDirectory, AssetDatabase& assetDatabase)
    {
        if (!ImGui::BeginDragDropTarget())
        {
            return;
        }

        std::filesystem::path l_Source;

        // Generic path payload: folders and non-asset files carry their path directly.
        if (const ImGuiPayload* l_Payload = ImGui::AcceptDragDropPayload(k_ContentPathPayload))
        {
            l_Source = std::string(static_cast<const char*>(l_Payload->Data));
        }
        // Typed asset payloads (unchanged, still used by the Inspector): resolve the UUID back to a path.
        else if (const ImGuiPayload* l_MeshPayload = ImGui::AcceptDragDropPayload("TRINITY_ASSET_MESH"))
        {
            uint64_t l_Raw = *static_cast<const uint64_t*>(l_MeshPayload->Data);
            if (const AssetMetadata* l_Meta = assetDatabase.GetMetadata(UUID(l_Raw)))
            {
                l_Source = m_Engine.GetPlatform().GetFileSystem().Resolve(BaseDirectory::Executable, l_Meta->SourcePath);
            }
        }
        else if (const ImGuiPayload* l_MaterialPayload = ImGui::AcceptDragDropPayload("TRINITY_ASSET_MATERIAL"))
        {
            uint64_t l_Raw = *static_cast<const uint64_t*>(l_MaterialPayload->Data);
            if (const AssetMetadata* l_Meta = assetDatabase.GetMetadata(UUID(l_Raw)))
            {
                l_Source = m_Engine.GetPlatform().GetFileSystem().Resolve(BaseDirectory::Executable, l_Meta->SourcePath);
            }
        }

        if (!l_Source.empty())
        {
            MoveEntry(l_Source, targetDirectory, assetDatabase);
        }

        ImGui::EndDragDropTarget();
    }

    void ContentBrowserPanel::RenderItemContextMenu(const std::filesystem::path& path, AssetDatabase& assetDatabase)
    {
        if (ImGui::BeginPopupContextItem("##ItemContext"))
        {
            m_SelectedAsset = path;

            if (ImGui::MenuItem(ICON_FA_PEN "  Rename"))
            {
                BeginRename(path);
            }

            if (ImGui::MenuItem(ICON_FA_TRASH "  Delete"))
            {
                m_PendingDelete = path;
                m_OpenDeleteModal = true;
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Show in Explorer"))
            {
                OpenInFileExplorer(path);
            }

            ImGui::EndPopup();
        }
    }

    void ContentBrowserPanel::RenderTileLabel(const std::filesystem::path& path, const std::string& name, float thumbnail, AssetDatabase& assetDatabase)
    {
        if (m_RenamingAsset != path)
        {
            CenteredClippedText(name, thumbnail);

            return;
        }

        ImGui::SetNextItemWidth(thumbnail);

        if (m_RenameRequestFocus)
        {
            ImGui::SetKeyboardFocusHere();
            m_RenameRequestFocus = false;
        }

        if (ImGui::InputText("##RenameField", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            CommitRename(assetDatabase);
        }
        else if (ImGui::IsItemDeactivated())
        {
            m_RenamingAsset.clear();
        }
    }

    void ContentBrowserPanel::RenderFooter()
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d item%s", m_AssetCount, m_AssetCount == 1 ? "" : "s");

        float l_SliderWidth = ImGui::GetFontSize() * 8.0f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - l_SliderWidth);
        ImGui::SetNextItemWidth(l_SliderWidth);
        ImGui::SliderFloat("##ThumbnailZoom", &m_ThumbnailScale, 0.5f, 2.0f, "");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Thumbnail size");
        }
    }

    void ContentBrowserPanel::RenderDeleteModal(AssetDatabase& assetDatabase)
    {
        if (m_OpenDeleteModal)
        {
            ImGui::OpenPopup("Delete Asset?");
            m_OpenDeleteModal = false;
        }

        ImVec2 l_Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(l_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Delete '%s'?", m_PendingDelete.filename().string().c_str());
            ImGui::TextDisabled("This action cannot be undone.");
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.24f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.30f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.20f, 0.20f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f)))
            {
                std::error_code l_Error;
                std::filesystem::remove_all(m_PendingDelete, l_Error);

                std::filesystem::path l_Meta = m_PendingDelete;
                l_Meta += ".meta";
                std::error_code l_MetaError;
                std::filesystem::remove(l_Meta, l_MetaError);

                if (m_SelectedAsset == m_PendingDelete)
                {
                    m_SelectedAsset.clear();
                }

                m_PendingDelete.clear();
                assetDatabase.Refresh();
                ReResolveModifiedMeshes();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                m_PendingDelete.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void ContentBrowserPanel::BeginRename(const std::filesystem::path& path)
    {
        m_RenamingAsset = path;

        std::string l_Current = std::filesystem::is_directory(path) ? path.filename().string() : path.stem().string();
        std::strncpy(m_RenameBuffer, l_Current.c_str(), sizeof(m_RenameBuffer) - 1);
        m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';

        m_RenameRequestFocus = true;
    }

    void ContentBrowserPanel::CommitRename(AssetDatabase& assetDatabase)
    {
        if (m_RenamingAsset.empty())
        {
            return;
        }

        std::string l_NewName = m_RenameBuffer;
        if (!l_NewName.empty())
        {
            std::filesystem::path l_Target = m_RenamingAsset.parent_path() / l_NewName;
            if (!std::filesystem::is_directory(m_RenamingAsset))
            {
                l_Target += m_RenamingAsset.extension();
            }

            std::error_code l_ExistsError;
            if (l_Target != m_RenamingAsset && !std::filesystem::exists(l_Target, l_ExistsError))
            {
                std::error_code l_RenameError;
                std::filesystem::rename(m_RenamingAsset, l_Target, l_RenameError);

                std::filesystem::path l_Meta = m_RenamingAsset;
                l_Meta += ".meta";
                std::error_code l_MetaError;
                if (std::filesystem::exists(l_Meta, l_MetaError))
                {
                    std::filesystem::path l_NewMeta = l_Target;
                    l_NewMeta += ".meta";
                    std::filesystem::rename(l_Meta, l_NewMeta, l_MetaError);
                }

                if (!l_RenameError)
                {
                    if (m_SelectedAsset == m_RenamingAsset)
                    {
                        m_SelectedAsset = l_Target;
                    }

                    assetDatabase.Refresh();
                    ReResolveModifiedMeshes();
                }
            }
        }

        m_RenamingAsset.clear();
    }

    void ContentBrowserPanel::CreateNewFolder()
    {
        std::string l_Base = "NewFolder";
        std::filesystem::path l_Path = m_CurrentDirectory / l_Base;
        uint32_t l_Index = 1;

        std::error_code l_ExistsError;
        while (std::filesystem::exists(l_Path, l_ExistsError))
        {
            l_Path = m_CurrentDirectory / (l_Base + "_" + std::to_string(l_Index++));
        }

        std::error_code l_CreateError;
        std::filesystem::create_directory(l_Path, l_CreateError);
        if (!l_CreateError)
        {
            m_SelectedAsset = l_Path;
            BeginRename(l_Path);
        }
    }

    void ContentBrowserPanel::ImportAssets()
    {
        std::filesystem::path l_Destination = m_CurrentDirectory;

        std::vector<FileFilter> l_Filters =
        {
            { "Assets", "gltf;glb;obj;fbx;png;jpg;jpeg;tga;hdr;wav;ogg;mp3;material" },
            { "All Files", "*" }
        };

        m_Engine.GetPlatform().OpenFileDialog(l_Filters, true, l_Destination, [this, l_Destination](const std::vector<std::filesystem::path>& files)
            {
                if (files.empty())
                {
                    return;
                }

                for (const std::filesystem::path& it_File : files)
                {
                    std::error_code l_Error;
                    std::filesystem::copy_file(it_File, l_Destination / it_File.filename(), std::filesystem::copy_options::overwrite_existing, l_Error);
                }

                if (m_Engine.HasAssetDatabase())
                {
                    m_Engine.GetAssetDatabase().Refresh();
                    ReResolveModifiedMeshes();
                }
            });
    }
}