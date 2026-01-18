#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace Engine
{
    class Event;

    struct WindowProperties
    {
        std::string Title = "PhysicsEngine";
        uint32_t Width = 1920;
        uint32_t Height = 1080;
        bool VSync = true;

        WindowProperties() = default;
        WindowProperties(std::string title, uint32_t width, uint32_t height) : Title(std::move(title)), Width(width), Height(height)
        {

        }
    };

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual bool ShouldClose() const = 0;

        virtual void* GetNativeWindow() const = 0;

        static std::unique_ptr<Window> Create(const WindowProperties& props = WindowProperties());
    };
}