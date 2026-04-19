#pragma once

#include "Trinity/Events/EventQueue.h"
#include "Trinity/Platform/Window/Window.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Trinity
{
    class DesktopWindow final : public Window
    {
    public:
        DesktopWindow();
        ~DesktopWindow() override;

        void Initialize(const WindowProperties& properties = WindowProperties()) override;
        void Shutdown() override;

        void OnUpdate() override;
        void OnEvent(Event& e) override;

        EventQueue& GetEventQueue() override { return m_EventQueue; }

        uint32_t GetWidth() const override { return m_Data.Width; }
        uint32_t GetHeight() const override { return m_Data.Height; }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override { return m_Data.VSync; }

        void SetTitle(const std::string& title) override;

        void SetCursorVisible(bool visible) override;
        void SetCursorLocked(bool locked) override;
        bool IsCursorVisible() const override { return m_CursorVisible; }
        bool IsCursorLocked() const override { return m_CursorLocked; }

        bool ShouldClose() const override { return m_ShouldClose; }
        bool IsMinimized() const override { return m_Data.Minimized; }

        NativeWindowHandle GetNativeHandle() const override;

    private:
        struct WindowData
        {
            std::string Title;
            uint32_t Width = 0;
            uint32_t Height = 0;
            bool VSync = true;
            bool Resizable = true;
            bool Minimized = false;
        };

    private:
        SDL_Window* m_WindowHandle = nullptr;
        uint32_t m_WindowID = 0;
        std::unordered_map<int, SDL_Gamepad*> m_Gamepads{};

        float m_CursorRestoreX = 0.0f;
        float m_CursorRestoreY = 0.0f;
        bool m_HasCursorRestorePos = false;

        WindowData m_Data{};
        EventQueue m_EventQueue{};

        bool m_Initialized = false;
        bool m_ShouldClose = false;
        bool m_CursorVisible = true;
        bool m_CursorLocked = false;
    };
}