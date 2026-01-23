#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace Engine
{
    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class WindowFocusEvent;
    class WindowLostFocusEvent;
    class WindowMovedEvent;

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

        void OnEvent(Event& e);

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetWidth(uint32_t width) = 0;
        virtual void SetHeight(uint32_t height) = 0;

        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual bool ShouldClose() const = 0;
        bool IsMinimized() const { return m_Minimized; }

        virtual void* GetNativeWindow() const = 0;

        static std::unique_ptr<Window> Create(const WindowProperties& properties = WindowProperties());

    protected:
        virtual bool OnWindowClose(WindowCloseEvent& e);
        virtual bool OnWindowResize(WindowResizeEvent& e);
        virtual bool OnWindowFocus(WindowFocusEvent& e);
        virtual bool OnWindowLostFocus(WindowLostFocusEvent& e);
        virtual bool OnWindowMoved(WindowMovedEvent& e);

        bool m_ShouldClose = false;
        bool m_Minimized = false;
    };
}