#pragma once

#include "Forge/Panels/Panel.h"

#include "Trinity/Renderer/Resources/Texture.h"

#include <imgui.h>

#include <array>
#include <filesystem>
#include <memory>

namespace Forge
{
    enum class FileIconType : uint8_t
    {
        Folder = 0,
        Scene,
        Mesh,
        Texture,
        Audio,
        Script,
        Prefab,
        Generic,
        Count
    };

    struct FileIcon
    {
        std::shared_ptr<Trinity::Texture> Texture;
        uint64_t ImGuiID = 0;
        ImU32 Color = IM_COL32(120, 120, 120, 255);
        const char* Label = "?";
    };

    class ContentBrowserPanel : public Panel
    {
    public:
        explicit ContentBrowserPanel(std::string name);

        void OnInitialize() override;
        void OnShutdown() override;
        void OnRender() override;

    private:
        void LoadIcons();
        void RenderEntry(const std::string& name, FileIconType iconType, bool isDirectory, bool& outDoubleClicked);

        static std::vector<uint8_t> MakeIconPixels(uint8_t r, uint8_t g, uint8_t b);
        static std::vector<uint8_t> MakeFolderPixels(uint8_t r, uint8_t g, uint8_t b);

        FileIconType FileTypeFromExtension(const std::string& ext) const;

    private:
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;

        static constexpr size_t s_IconCount = static_cast<size_t>(FileIconType::Count);
        std::array<FileIcon, s_IconCount> m_Icons;

        bool m_IconsLoaded = false;

        static constexpr float s_IconSize = 20.0f;
        static constexpr float s_IconPadding = 4.0f;
        static constexpr float s_RowHeight = s_IconSize + 6.0f;
    };
}
