#include "Trinity/Utilities/FileManagement.h"
#include "Trinity/Utilities/Log.h"

#include <fstream>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>
#include <wrl/client.h>
#endif

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

#ifdef _WIN32
        static std::vector<COMDLG_FILTERSPEC> BuildFilterSpecs(const std::vector<FileDialogFilter>& filters, std::vector<std::wstring>& wideStorage)
        {
            std::vector<COMDLG_FILTERSPEC> l_Specs;
            l_Specs.reserve(filters.size());
            wideStorage.reserve(filters.size() * 2);

            for (const FileDialogFilter& it_Filter : filters)
            {
                std::wstring& l_Name = wideStorage.emplace_back(it_Filter.Name.begin(), it_Filter.Name.end());
                std::wstring& l_Spec = wideStorage.emplace_back(it_Filter.Spec.begin(), it_Filter.Spec.end());
                l_Specs.push_back({ l_Name.c_str(), l_Spec.c_str() });
            }

            return l_Specs;
        }

        static std::optional<std::string> RunFileDialog(bool open, const std::vector<FileDialogFilter>& filters, const std::string& defaultExtension)
        {
            Microsoft::WRL::ComPtr<IFileDialog> l_Dialog;

            const CLSID l_Clsid = open ? CLSID_FileOpenDialog : CLSID_FileSaveDialog;
            const HRESULT l_CreateResult = CoCreateInstance(l_Clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&l_Dialog));
            if (FAILED(l_CreateResult))
            {
                TR_CORE_ERROR("FileManagement: failed to create IFileDialog (HRESULT {:#x})", static_cast<uint32_t>(l_CreateResult));

                return std::nullopt;
            }

            std::vector<std::wstring> l_WideStorage;
            const std::vector<COMDLG_FILTERSPEC> l_Specs = BuildFilterSpecs(filters, l_WideStorage);

            if (!l_Specs.empty())
            {
                l_Dialog->SetFileTypes(static_cast<UINT>(l_Specs.size()), l_Specs.data());
                l_Dialog->SetFileTypeIndex(1);
            }

            if (!defaultExtension.empty())
            {
                const std::wstring l_WideExt(defaultExtension.begin(), defaultExtension.end());
                l_Dialog->SetDefaultExtension(l_WideExt.c_str());
            }

            const HRESULT l_ShowResult = l_Dialog->Show(nullptr);
            if (FAILED(l_ShowResult))
            {
                return std::nullopt;
            }

            Microsoft::WRL::ComPtr<IShellItem> l_Item;
            if (FAILED(l_Dialog->GetResult(&l_Item)))
            {
                return std::nullopt;
            }

            PWSTR l_RawPath = nullptr;
            if (FAILED(l_Item->GetDisplayName(SIGDN_FILESYSPATH, &l_RawPath)))
            {
                return std::nullopt;
            }

            const std::wstring l_WidePath(l_RawPath);
            CoTaskMemFree(l_RawPath);

            return std::string(l_WidePath.begin(), l_WidePath.end());
        }
#endif

        std::optional<std::string> FileManagement::OpenFileDialog(const std::vector<FileDialogFilter>& filters)
        {
#ifdef _WIN32
            return RunFileDialog(true, filters, "");
#else
            TR_CORE_WARN("FileManagement::OpenFileDialog: not implemented on this platform");

            return std::nullopt;
#endif
        }

        std::optional<std::string> FileManagement::SaveFileDialog(const std::vector<FileDialogFilter>& filters, const std::string& defaultExtension)
        {
#ifdef _WIN32
            return RunFileDialog(false, filters, defaultExtension);
#else
            TR_CORE_WARN("FileManagement::SaveFileDialog: not implemented on this platform");

            return std::nullopt;
#endif
        }

    }
}