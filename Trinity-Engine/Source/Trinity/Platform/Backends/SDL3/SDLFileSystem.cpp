#include <Trinity/Platform/Backends/SDL3/SDLFileSystem.h>

#include <SDL3/SDL_filesystem.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    SDLFileSystem::SDLFileSystem()
    {
        const char* l_Base = SDL_GetBasePath();
        if (l_Base != nullptr)
        {
            m_BasePath = std::filesystem::path(l_Base);
        }
        else
        {
            TR_CORE_WARN("SDLFileSystem: SDL_GetBasePath returned null, using current path");
            m_BasePath = std::filesystem::current_path();
        }
    }

    std::filesystem::path SDLFileSystem::GetBaseDirectory(BaseDirectory base) const
    {
        switch (base)
        {
            case BaseDirectory::Executable:
                return m_BasePath;

            case BaseDirectory::Working:
                return std::filesystem::current_path();

            case BaseDirectory::Temp:
            {
                std::error_code l_Error;
                return std::filesystem::temp_directory_path(l_Error);
            }

            case BaseDirectory::UserData:
            case BaseDirectory::UserConfig:
            case BaseDirectory::UserCache:
            {
                char* l_Pref = SDL_GetPrefPath(m_Organization.c_str(), m_ApplicationName.c_str());
                if (l_Pref == nullptr)
                {
                    TR_CORE_WARN("SDLFileSystem: SDL_GetPrefPath failed, falling back to executable path");
                    return m_BasePath;
                }

                std::filesystem::path l_Root(l_Pref);
                SDL_free(l_Pref);

                switch (base)
                {
                    case BaseDirectory::UserData:
                        return l_Root / "Data";
                    case BaseDirectory::UserConfig:
                        return l_Root / "Config";
                    case BaseDirectory::UserCache:
                        return l_Root / "Cache";
                    default:
                        return l_Root;
                }
            }
        }

        return m_BasePath;
    }

    std::filesystem::path SDLFileSystem::Resolve(BaseDirectory base, const std::filesystem::path& relative) const
    {
        return GetBaseDirectory(base) / relative;
    }
}