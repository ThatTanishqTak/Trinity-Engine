#pragma once

#include "Engine/Platform/Window.h"

#include <array>

struct GLFWwindow;

namespace Engine
{
    class GLFWWindow final : public Window
    {
    public:
        explicit GLFWWindow(const WindowProperties& props);
        ~GLFWWindow() override;

        void OnUpdate() override;

        uint32_t GetWidth() const override { return m_Data.Width; }
        uint32_t GetHeight() const override { return m_Data.Height; }

        void SetWidth(uint32_t width) override { m_Data.Width = width; }
        void SetHeight(uint32_t height) override { m_Data.Height = height; }

        void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override { return m_Data.VSync; }

        bool ShouldClose() const override;

        void* GetNativeWindow() const override { return m_Window; }

    private:
        void Initialize(const WindowProperties& props);
        void Shutdown();

        void UpdateGamepads();

    private:
        GLFWwindow* m_Window = nullptr;

        struct WindowData
        {
            std::string Title;
            uint32_t Width = 0;
            uint32_t Height = 0;
            bool VSync = true;

            bool FramebufferResized = false;
            EventCallbackFn EventCallback;
        };

        WindowData m_Data;

        // GLFW_JOYSTICK_LAST is typically 15 (so 16 total)
        static constexpr int s_MaxGamepads = 16;
        static constexpr int s_MaxAxes = 6;     // GLFW_GAMEPAD_AXIS_LAST + 1
        static constexpr int s_MaxButtons = 15; // GLFW_GAMEPAD_BUTTON_LAST + 1

        struct GamepadData
        {
            bool Present = false;
            bool HasState = false;

            std::array<float, s_MaxAxes> Axes{};
            std::array<unsigned char, s_MaxButtons> Buttons{};
        };

        std::array<GamepadData, s_MaxGamepads> m_Gamepads{};
    };
}