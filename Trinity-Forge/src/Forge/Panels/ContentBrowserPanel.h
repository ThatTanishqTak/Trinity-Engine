#pragma once

#include "Forge/Panels/Panel.h"

#include <filesystem>

namespace Forge
{
    class ContentBrowserPanel : public Panel
    {
    public:
        explicit ContentBrowserPanel(std::string name);

        void OnRender() override;

    private:
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
    };
}
