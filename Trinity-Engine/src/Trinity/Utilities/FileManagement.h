#pragma once

#include <optional>
#include <string>
#include <vector>

namespace Trinity
{
    namespace Utilities
    {
        struct FileDialogFilter
        {
            std::string Name;
            std::string Spec;
        };

        class FileManagement
        {
        public:
            static std::vector<char> LoadFromFile(const std::string& path);
            static void SaveToFile(const std::string& path, const std::vector<char>& data);

            static std::optional<std::string> OpenFileDialog(const std::vector<FileDialogFilter>& filters = {});
            static std::optional<std::string> SaveFileDialog(const std::vector<FileDialogFilter>& filters = {}, const std::string& defaultExtension = "");
        };
    }
}