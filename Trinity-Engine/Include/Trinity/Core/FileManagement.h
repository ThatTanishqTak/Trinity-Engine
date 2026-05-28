#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <filesystem>

namespace Trinity
{
    class FileManagement
    {
    public:
        static std::optional<std::string> ReadText(const std::filesystem::path& path);
        static std::optional<std::vector<uint8_t>> ReadBinary(const std::filesystem::path& path);

        static bool WriteText(const std::filesystem::path& path, const std::string& content);
        static bool WriteBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data);

        static bool Exists(const std::filesystem::path& path);
        static bool EnsureDirectory(const std::filesystem::path& path);
    };
}