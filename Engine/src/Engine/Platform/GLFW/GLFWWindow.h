#pragma once

#include "Engine/Platform/Window.h"

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

        void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override { return m_Data.VSync; }

        bool ShouldClose() const override;

        void* GetNativeWindow() const override { return m_Window; }

    private:
        void Initialize(const WindowProperties& props);
        void Shutdown();

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
    };
}