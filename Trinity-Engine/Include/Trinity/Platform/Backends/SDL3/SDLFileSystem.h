#pragma once

#include <string>

#include <Trinity/Platform/FileSystem.h>

namespace Trinity
{
    class SDLFileSystem : public FileSystem
    {
    public:
        SDLFileSystem();

        std::filesystem::path GetBaseDirectory(BaseDirectory base) const override;
        std::filesystem::path Resolve(BaseDirectory base, const std::filesystem::path& relative) const override;

        std::string GetApplicationName() const override { return m_ApplicationName; }
        void SetApplicationName(const std::string& name) override { m_ApplicationName = name; }

    private:
        std::filesystem::path m_BasePath;
        std::string m_ApplicationName = "Trinity";
        std::string m_Organization = "Trinity";
    };
}