#pragma once

#include <vector>
#include <string>

namespace Trinity
{
    namespace Utilities
    {
        class FileManagement
        {
        public:
            static std::vector<char> LoadFromFile(const std::string& path);
            static void SaveToFile(const std::string& path, const std::vector<char>& data);
        };
    }
}