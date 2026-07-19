#include <Trinity/Core/FileManagement.h>

#include <fstream>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    std::optional<std::string> FileManagement::ReadText(const std::filesystem::path& path)
    {
        std::ifstream l_Stream(path, std::ios::in | std::ios::binary);
        if (!l_Stream)
        {
            ("Failed to open: {}", path.string());
            return std::nullopt;
        }

        std::string l_Content;
        l_Stream.seekg(0, std::ios::end);
        l_Content.resize(static_cast<size_t>(l_Stream.tellg()));
        l_Stream.seekg(0, std::ios::beg);
        l_Stream.read(l_Content.data(), static_cast<std::streamsize>(l_Content.size()));

        return l_Content;
    }

    std::optional<std::vector<uint8_t>> FileManagement::ReadBinary(const std::filesystem::path& path)
    {
        std::ifstream l_Stream(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!l_Stream)
        {
            ("Failed to open: {}", path.string());
            return std::nullopt;
        }

        std::streamsize l_Size = l_Stream.tellg();
        l_Stream.seekg(0, std::ios::beg);

        std::vector<uint8_t> l_Data(static_cast<size_t>(l_Size));
        l_Stream.read(reinterpret_cast<char*>(l_Data.data()), l_Size);

        return l_Data;
    }

    bool FileManagement::WriteText(const std::filesystem::path& path, const std::string& content)
    {
        std::ofstream l_Stream(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!l_Stream)
        {
            ("Failed to open: {}", path.string());
            return false;
        }

        l_Stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        return true;
    }

    bool FileManagement::WriteBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data)
    {
        std::ofstream l_Stream(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!l_Stream)
        {
            ("Failed to open: {}", path.string());
            return false;
        }

        l_Stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        return true;
    }

    bool FileManagement::Exists(const std::filesystem::path& path)
    {
        return std::filesystem::exists(path);
    }

    bool FileManagement::EnsureDirectory(const std::filesystem::path& path)
    {
        if (std::filesystem::exists(path))
        {
            return std::filesystem::is_directory(path);
        }

        std::error_code l_Error;
        bool l_Created = std::filesystem::create_directories(path, l_Error);
        if (!l_Created)
        {
            ("Failed for {}: {}", path.string(), l_Error.message());
        }

        return l_Created;
    }
}