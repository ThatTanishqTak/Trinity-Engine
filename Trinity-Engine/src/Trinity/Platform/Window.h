#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace Trinity
{
    class Event;
    class EventQueue;

    enum class NativeWindowType : uint8_t
    {
        None = 0,
        Win32,
        X11,
        Wayland,
        Cocoa
    };

    struct NativeWindowHandle
    {
        NativeWindowType Type = NativeWindowType::None;

        void* Window = nullptr;    // HWND on Win32, xcb_window_t on XCB, wl_surface* on Wayland, NSWindow* on Cocoa
        void* Instance = nullptr;  // HINSTANCE on Win32
        void* Display = nullptr;   // Display* on X11, xcb_connection_t* on XCB, wl_display* on Wayland
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

        virtual void SetCursorVisible(bool visible) = 0;
        virtual void SetCursorLocked(bool locked) = 0;
        virtual bool IsCursorVisible() const = 0;
        virtual bool IsCursorLocked() const = 0;

        virtual bool ShouldClose() const = 0;
        virtual bool IsMinimized() const = 0;

        virtual NativeWindowHandle GetNativeHandle() const = 0;

        static std::unique_ptr<Window> Create();

    protected:
        Window() = default;
    };
}