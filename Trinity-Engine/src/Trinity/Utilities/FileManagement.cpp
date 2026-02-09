#include "Trinity/Utilities/FileManagement.h"
#include "Trinity/Utilities/Log.h"

#include <fstream>
#include <filesystem>

namespace Trinity
{
    namespace Utilities
    {
        std::vector<char> FileManagement::LoadFromFile(const std::string& path)
        {
            std::ifstream l_File(path, std::ios::ate | std::ios::binary);
            if (!l_File.is_open())
            {
                TR_CORE_CRITICAL("Failed to open file: {}", path.c_str());

                std::abort();
            }

            const size_t l_FileSize = (size_t)l_File.tellg();
            std::vector<char> l_Buffer(l_FileSize);

            l_File.seekg(0);
            l_File.read(l_Buffer.data(), (std::streamsize)l_FileSize);
            l_File.close();

            return l_Buffer;
        }

        void FileManagement::SaveToFile(const std::string& path, const std::vector<char>& data)
        {
            const std::filesystem::path l_Path(path);
            const std::filesystem::path l_ParentPath = l_Path.parent_path();
            if (!l_ParentPath.empty())
            {
                std::error_code l_Error;
                std::filesystem::create_directories(l_ParentPath, l_Error);
            }

            std::ofstream l_File(path, std::ios::binary | std::ios::trunc);
            if (!l_File.is_open())
            {
                TR_CORE_ERROR("Failed to open file for writing: {}", path.c_str());

                return;
            }

            if (!data.empty())
            {
                l_File.write(data.data(), static_cast<std::streamsize>(data.size()));
            }
        }
    }
}