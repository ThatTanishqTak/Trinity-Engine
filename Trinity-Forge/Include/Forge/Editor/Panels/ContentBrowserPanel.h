#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
        void RenderFolderTree(AssetDatabase& assetDatabase);
        void RenderFolderNode(const std::filesystem::path& directory, AssetDatabase& assetDatabase);
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
        void MoveEntry(const std::filesystem::path& source, const std::filesystem::path& targetDirectory, AssetDatabase& assetDatabase);
        void AcceptMoveInto(const std::filesystem::path& targetDirectory, AssetDatabase& assetDatabase);
        void PerformMove(const std::filesystem::path& source, const std::filesystem::path& destination, AssetDatabase& assetDatabase);
        void RenderMoveConflictModal(AssetDatabase& assetDatabase);
        void RefreshAssets(AssetDatabase& assetDatabase);
        uint64_t GetThumbnailTexture(const std::filesystem::path& file, AssetDatabase& assetDatabase);
        void ClearThumbnailCache();
        void ProcessRetiredThumbnails();

        std::filesystem::path m_AssetsRoot;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_SelectedAsset;
        std::filesystem::path m_RenamingAsset;
        std::filesystem::path m_PendingDelete;
        std::filesystem::path m_PendingMoveSource;
        std::filesystem::path m_PendingMoveTarget;
        bool m_OpenMoveConflictModal = false;
        std::unordered_map<uint64_t, uint64_t> m_ThumbnailCache;
        std::vector<std::pair<uint64_t, int>> m_RetiredThumbnails;
        bool m_Initialized = false;
        bool m_RenameRequestFocus = false;
        bool m_OpenDeleteModal = false;
        float m_ThumbnailScale = 2.5f;
        int m_AssetCount = 0;
        char m_SearchBuffer[128] = "";
        char m_RenameBuffer[128] = "";
    };
}