#pragma once

#include <string>
#include <filesystem>

namespace Trinity
{
    enum class BaseDirectory
    {
        Executable,
        Working,
        UserData,
        UserConfig,
        UserCache,
        Temp
    };

    class FileSystem
    {
    public:
        virtual ~FileSystem() = default;

        virtual std::filesystem::path GetBaseDirectory(BaseDirectory base) const = 0;

        virtual std::filesystem::path Resolve(BaseDirectory base, const std::filesystem::path& relative) const = 0;

        virtual std::string GetApplicationName() const = 0;
        virtual void SetApplicationName(const std::string& name) = 0;
    };
}