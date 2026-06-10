#pragma once

#include <filesystem>

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
        void ReResolveModifiedMeshes();

        std::filesystem::path m_AssetsRoot;
        std::filesystem::path m_CurrentDirectory;
        bool m_Initialized = false;
        char m_SearchBuffer[128] = "";
    };
}