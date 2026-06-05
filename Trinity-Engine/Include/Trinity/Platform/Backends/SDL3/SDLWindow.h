#pragma once

#include <string>
#include <cstdint>

#include <Trinity/Platform/Window.h>

struct SDL_Window;

namespace Trinity
{
    class SDLWindow : public Window
    {
    public:
        SDLWindow(const WindowProperties& properties);
        ~SDLWindow() override;

        void OnUpdate() override;

        uint32_t GetWidth() const override { return m_Data.Width; }
        uint32_t GetHeight() const override { return m_Data.Height; }

        void SetTitle(const std::string& title) override;
        const std::string& GetTitle() const override { return m_Data.Title; }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override { return m_Data.VSync; }

        bool IsMinimized() const override { return m_Data.Minimized; }

        void SetEventCallback(const EventCallback& callback) override { m_Data.Callback = callback; }

        void SetRelativeMouseMode(bool enabled) override;

        NativeWindowHandle GetNativeHandle() const override;

        SDL_Window* GetSDLWindow() const { return m_Window; }
        uint32_t GetWindowID() const;

        void DispatchEvent(Event& event);
        void SetSize(uint32_t width, uint32_t height);
        void SetMinimized(bool minimized);

    private:
        struct WindowData
        {
            std::string Title;
            
            uint32_t Width = 0;
            uint32_t Height = 0;
            
            bool VSync = true;
            bool Minimized = false;
            
            EventCallback Callback;
        };

        SDL_Window* m_Window = nullptr;
        WindowData m_Data;
    };
}