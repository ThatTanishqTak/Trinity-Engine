#pragma once

#include "Trinity/Assets/AssetRegistry.h"

#include <filesystem>

class ContentBrowserPanel
{
public:
    ContentBrowserPanel() = default;

    void OnImGuiRender();
    void SetRootPath(const std::filesystem::path& path);
    void Refresh();

private:
    void DrawCategoryList();
    void DrawAssetList();

private:
    Trinity::AssetType m_TypeFilter = Trinity::AssetType::Unknown;
    char m_SearchBuffer[256]{};
};