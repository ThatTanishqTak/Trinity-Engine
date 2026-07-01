#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <Trinity/Platform/PlatformTypes.h>
#include <Trinity/Platform/Window.h>
#include <Trinity/Platform/Input.h>
#include <Trinity/Platform/Gamepad.h>
#include <Trinity/Platform/FileSystem.h>

namespace Trinity
{
    class IImGuiPlatformBackend;

    struct FileFilter
    {
        std::string Name;
        std::string Pattern;
    };

    using FileDialogCallback = std::function<void(const std::vector<std::filesystem::path>&)>;

    class IPlatform
    {
    public:
        virtual ~IPlatform() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void PollEvents() = 0;

        virtual Window& CreateWindow(const WindowProperties& properties) = 0;
        virtual Window& GetWindow() = 0;
        virtual bool HasWindow() const = 0;

        virtual Input& GetInput() = 0;
        virtual Gamepad& GetGamepad() = 0;
        virtual FileSystem& GetFileSystem() = 0;
        virtual IImGuiPlatformBackend& GetImGuiBackend() = 0;

        // Shows a native asynchronous file-open dialog. The callback is invoked on the main thread during event polling with the selected paths (empty if cancelled)
        virtual void OpenFileDialog(const std::vector<FileFilter>& filters, bool allowMany, const std::filesystem::path& defaultLocation, const FileDialogCallback& callback) = 0;

        virtual PlatformType GetType() const = 0;
    };
}