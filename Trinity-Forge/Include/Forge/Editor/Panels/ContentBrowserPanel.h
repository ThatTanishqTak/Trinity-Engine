#pragma once

#include <filesystem>
#include <string>

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class AssetDatabase;

    class ContentBrowserPanel : public EditorPanel
    {
    public:
        ContentBrowserPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;
        void RenderContents();

    private:
        void EnsureRoot();
        void RenderToolbar(AssetDatabase& assetDatabase);
        void RenderBreadcrumbs();
        void RenderFolderTree();
        void RenderFolderNode(const std::filesystem::path& directory);
        void RenderGrid(AssetDatabase& assetDatabase);
        void RenderItemContextMenu(const std::filesystem::path& path, AssetDatabase& assetDatabase);
        void RenderTileLabel(const std::filesystem::path& path, const std::string& name, float thumbnail, AssetDatabase& assetDatabase);
        void RenderFooter();
        void RenderDeleteModal(AssetDatabase& assetDatabase);
        void BeginRename(const std::filesystem::path& path);
        void CommitRename(AssetDatabase& assetDatabase);
        void CreateNewFolder();
        void ImportAssets();
        void ReResolveModifiedMeshes();

        std::filesystem::path m_AssetsRoot;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_SelectedAsset;
        std::filesystem::path m_RenamingAsset;
        std::filesystem::path m_PendingDelete;
        bool m_Initialized = false;
        bool m_RenameRequestFocus = false;
        bool m_OpenDeleteModal = false;
        float m_ThumbnailScale = 2.0f;
        int m_AssetCount = 0;
        char m_SearchBuffer[128] = "";
        char m_RenameBuffer[128] = "";
    };
}