#pragma once

#include <string>
#include <cstdint>
#include <functional>

#include <Trinity/Core/Event.h>
#include <Trinity/Platform/PlatformTypes.h>

namespace Trinity
{
    struct WindowProperties
    {
        std::string Title = "Trinity";
        
        uint32_t Width = 1280;
        uint32_t Height = 720;
        
        bool Resizable = true;
        bool VSync = true;
    };

    class Window
    {
    public:
        using EventCallback = std::function<void(Event&)>;

        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetTitle(const std::string& title) = 0;
        virtual const std::string& GetTitle() const = 0;

        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual bool IsMinimized() const = 0;

        virtual void SetEventCallback(const EventCallback& callback) = 0;

        virtual void SetRelativeMouseMode(bool enabled) = 0;

        virtual NativeWindowHandle GetNativeHandle() const = 0;
    };
}