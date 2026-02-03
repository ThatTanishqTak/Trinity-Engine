#pragma once

#include "Trinity/Platform/Window.h"
#include "Trinity/Input/InputCodes.h"

#include <cstdint>
#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace Trinity
{
    class WindowsWindow final : public Window
    {
    public:
        WindowsWindow();
        ~WindowsWindow() override;

        void Initialize(const WindowProperties& properties = WindowProperties()) override;
        void Shutdown() override;

        void OnUpdate() override;
        void OnEvent(Event& e) override;

        void SetEventCallback(const EventCallbackFn& callback) override;

        uint32_t GetWidth() const override { return m_Data.Width; }
        uint32_t GetHeight() const override { return m_Data.Height; }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override { return m_Data.VSync; }

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

            EventCallbackFn EventCallback;
        };

    private:
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        static Code::KeyCode TranslateKeyCode(WPARAM wParam, LPARAM lParam);
        static Code::MouseCode TranslateMouseButton(UINT msg, WPARAM wParam);

        void DispatchEvent(Event& e);

        void RegisterWindowClassIfNeeded();
        void CreateNativeWindow();
        void DestroyNativeWindow();

    private:
        HINSTANCE m_InstanceHandle = nullptr;
        HWND m_WindowHandle = nullptr;

        WindowData m_Data{};
        bool m_Initialized = false;
        bool m_ShouldClose = false;
    };
}