#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <SDL3/SDL_video.h>

namespace Trinity
{
    class Event;
    class EventQueue;

    struct NativeWindowHandle
    {
        SDL_Window* Window = nullptr;
        SDL_PropertiesID Properties = 0;
    };

    struct WindowProperties
    {
        std::string Title = "Trinity-Window";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool VSync = true;
        bool Resizable = true;
    };

    class Window
    {
    public:
        virtual ~Window() = default;

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        virtual void Initialize(const WindowProperties& properties = WindowProperties()) = 0;
        virtual void Shutdown() = 0;

        virtual void OnUpdate() = 0;
        virtual void OnEvent(Event& e) = 0;

        virtual EventQueue& GetEventQueue() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void SetTitle(const std::string& title) = 0;

        virtual void SetCursorVisible(bool visible) = 0;
        virtual void SetCursorLocked(bool locked) = 0;
        virtual bool IsCursorVisible() const = 0;
        virtual bool IsCursorLocked() const = 0;

        virtual bool ShouldClose() const = 0;
        virtual bool IsMinimized() const = 0;

        virtual NativeWindowHandle GetNativeHandle() const = 0;

        void SetPlatformEventCallback(std::function<void(const void*)> callback) { m_PlatformEventCallback = std::move(callback); }

        static std::unique_ptr<Window> Create();

    protected:
        Window() = default;

        std::function<void(const void*)> m_PlatformEventCallback;
    };
}