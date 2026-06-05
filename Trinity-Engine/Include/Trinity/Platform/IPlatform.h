#pragma once

#include <Trinity/Platform/PlatformTypes.h>
#include <Trinity/Platform/Window.h>
#include <Trinity/Platform/Input.h>
#include <Trinity/Platform/Gamepad.h>
#include <Trinity/Platform/FileSystem.h>

namespace Trinity
{
    class IImGuiPlatformBackend;

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

        virtual PlatformType GetType() const = 0;
    };
}