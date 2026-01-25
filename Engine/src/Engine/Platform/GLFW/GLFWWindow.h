#pragma once

#include "Engine/Platform/Window.h"

#include "Engine/Utilities/Utilities.h"

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

        void SetEventCallback(const EventCallbackFn& callback) override
        {
            TR_CORE_TRACE("Setting up window events");

            m_Data.EventCallback = callback;

            TR_CORE_TRACE("Window events setup complete");
        }

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

        static constexpr int s_MaxGamepads = 16;
        static constexpr int s_MaxAxes = 6;
        static constexpr int s_MaxButtons = 15;

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