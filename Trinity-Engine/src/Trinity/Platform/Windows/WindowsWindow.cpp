#include "Trinity/Platform/Windows/WindowsWindow.h"

#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"
#include "Trinity/Platform/SDL/SDLEventTranslator.h"
#include "Trinity/Utilities/Log.h"

#include <memory>

namespace Trinity
{
    WindowsWindow::WindowsWindow()
    {

    }

    WindowsWindow::~WindowsWindow()
    {

    }

    void WindowsWindow::Initialize(const WindowProperties& properties)
    {
        if (m_Initialized)
        {
            TR_CORE_WARN("WindowsWindow::Initialize called more than once. Ignoring.");

            return;
        }

        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            TR_CORE_CRITICAL("SDL_InitSubSystem(SDL_INIT_VIDEO) failed: {}", SDL_GetError());

            return;
        }

        m_Data.Title = properties.Title;
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Height;
        m_Data.VSync = properties.VSync;
        m_Data.Resizable = properties.Resizable;
        m_Data.Minimized = (properties.Width == 0 || properties.Height == 0);

        SDL_WindowFlags l_WindowFlags = SDL_WINDOW_VULKAN;
        if (m_Data.Resizable)
        {
            l_WindowFlags |= SDL_WINDOW_RESIZABLE;
        }

        m_WindowHandle = SDL_CreateWindow(m_Data.Title.c_str(), static_cast<int>(m_Data.Width), static_cast<int>(m_Data.Height), l_WindowFlags);
        if (m_WindowHandle == nullptr)
        {
            TR_CORE_CRITICAL("SDL_CreateWindow failed: {}", SDL_GetError());

            SDL_QuitSubSystem(SDL_INIT_VIDEO);

            return;
        }

        m_WindowID = SDL_GetWindowID(m_WindowHandle);

        m_ShouldClose = false;
        m_CursorVisible = true;
        m_CursorLocked = false;
        m_Initialized = true;
    }

    void WindowsWindow::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        SetCursorLocked(false);
        SetCursorVisible(true);

        if (m_WindowHandle != nullptr)
        {
            SDL_DestroyWindow(m_WindowHandle);
            m_WindowHandle = nullptr;
        }

        m_WindowID = 0;
        m_Initialized = false;

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    void WindowsWindow::OnUpdate()
    {
        if (!m_Initialized)
        {
            return;
        }

        SDL_Event l_Event{};
        while (SDL_PollEvent(&l_Event))
        {
            if (m_PlatformEventCallback)
            {
                m_PlatformEventCallback(&l_Event);
            }

            if (l_Event.type == SDL_EVENT_QUIT)
            {
                m_ShouldClose = true;

                continue;
            }

            if (l_Event.type == SDL_EVENT_WINDOW_RESIZED && l_Event.window.windowID == m_WindowID)
            {
                const uint32_t l_Width = static_cast<uint32_t>(l_Event.window.data1);
                const uint32_t l_Height = static_cast<uint32_t>(l_Event.window.data2);

                m_Data.Minimized = (l_Width == 0 || l_Height == 0);
                m_EventQueue.PushEvent(std::make_unique<WindowResizeEvent>(l_Width, l_Height));

                continue;
            }

            if (l_Event.type == SDL_EVENT_WINDOW_MINIMIZED && l_Event.window.windowID == m_WindowID)
            {
                m_Data.Minimized = true;

                continue;
            }

            if (l_Event.type == SDL_EVENT_WINDOW_RESTORED && l_Event.window.windowID == m_WindowID)
            {
                m_Data.Minimized = false;

                int l_Width = 0;
                int l_Height = 0;
                SDL_GetWindowSize(m_WindowHandle, &l_Width, &l_Height);
                m_EventQueue.PushEvent(std::make_unique<WindowResizeEvent>(static_cast<uint32_t>(l_Width), static_cast<uint32_t>(l_Height)));

                continue;
            }

            if (l_Event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && l_Event.window.windowID == m_WindowID)
            {
                m_EventQueue.PushEvent(std::make_unique<WindowCloseEvent>());

                continue;
            }

            if (l_Event.type == SDL_EVENT_KEY_DOWN && l_Event.key.windowID == m_WindowID)
            {
                const Code::KeyCode l_KeyCode = TranslateKeyCode(l_Event.key.key, l_Event.key.scancode);
                if (l_KeyCode != Code::KeyCode::UNKNOWN)
                {
                    const int l_RepeatCount = (l_Event.key.repeat == 0) ? 0 : 1;
                    m_EventQueue.PushEvent(std::make_unique<KeyPressedEvent>(l_KeyCode, l_RepeatCount));
                }

                continue;
            }

            if (l_Event.type == SDL_EVENT_KEY_UP && l_Event.key.windowID == m_WindowID)
            {
                const Code::KeyCode l_KeyCode = TranslateKeyCode(l_Event.key.key, l_Event.key.scancode);
                if (l_KeyCode != Code::KeyCode::UNKNOWN)
                {
                    m_EventQueue.PushEvent(std::make_unique<KeyReleasedEvent>(l_KeyCode));
                }

                continue;
            }

            if (l_Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && l_Event.button.windowID == m_WindowID)
            {
                const Code::MouseCode l_ButtonCode = TranslateMouseButton(l_Event.button.button);
                m_EventQueue.PushEvent(std::make_unique<MouseButtonPressedEvent>(l_ButtonCode));

                continue;
            }

            if (l_Event.type == SDL_EVENT_MOUSE_BUTTON_UP && l_Event.button.windowID == m_WindowID)
            {
                const Code::MouseCode l_ButtonCode = TranslateMouseButton(l_Event.button.button);
                m_EventQueue.PushEvent(std::make_unique<MouseButtonReleasedEvent>(l_ButtonCode));

                continue;
            }

            if (l_Event.type == SDL_EVENT_MOUSE_MOTION && l_Event.motion.windowID == m_WindowID)
            {
                m_EventQueue.PushEvent(std::make_unique<MouseMovedEvent>(l_Event.motion.x, l_Event.motion.y));
                m_EventQueue.PushEvent(std::make_unique<MouseRawDeltaEvent>(l_Event.motion.xrel, l_Event.motion.yrel));

                continue;
            }

            if (l_Event.type == SDL_EVENT_MOUSE_WHEEL && l_Event.wheel.windowID == m_WindowID)
            {
                m_EventQueue.PushEvent(std::make_unique<MouseScrolledEvent>(l_Event.wheel.x, l_Event.wheel.y));

                continue;
            }
        }
    }

    void WindowsWindow::OnEvent(Event& e)
    {
        EventDispatcher l_Dispatcher(e);

        l_Dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event)
            {
                m_Data.Width = event.GetWidth();
                m_Data.Height = event.GetHeight();
                m_Data.Minimized = (m_Data.Width == 0 || m_Data.Height == 0);

                return false;
            });

        l_Dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event)
            {
                if (event.Handled)
                {
                    return false;
                }

                m_ShouldClose = true;

                return false;
            });
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        m_Data.VSync = enabled;
    }

    void WindowsWindow::SetCursorVisible(bool visible)
    {
        if (m_CursorVisible == visible)
        {
            return;
        }

        m_CursorVisible = visible;

        if (visible)
        {
            SDL_ShowCursor();
        }
        else
        {
            SDL_HideCursor();
        }
    }

    void WindowsWindow::SetCursorLocked(bool locked)
    {
        if (m_CursorLocked == locked)
        {
            return;
        }

        if (locked)
        {
            SDL_GetGlobalMouseState(&m_CursorRestoreX, &m_CursorRestoreY);
            m_HasCursorRestorePos = true;
        }

        m_CursorLocked = locked;

        if (m_WindowHandle != nullptr)
        {
            SDL_SetWindowRelativeMouseMode(m_WindowHandle, locked);
        }

        if (!locked && m_HasCursorRestorePos && m_WindowHandle != nullptr)
        {
            SDL_WarpMouseInWindow(m_WindowHandle, m_CursorRestoreX, m_CursorRestoreY);
            m_HasCursorRestorePos = false;
        }
    }

    NativeWindowHandle WindowsWindow::GetNativeHandle() const
    {
        NativeWindowHandle l_Handle{};
        l_Handle.Window = m_WindowHandle;
        l_Handle.Properties = SDL_GetWindowProperties(m_WindowHandle);

#if defined(_WIN32)
        l_Handle.PlatformHandle = SDL_GetPointerProperty(l_Handle.Properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        l_Handle.PlatformHandle2 = SDL_GetPointerProperty(l_Handle.Properties, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
#endif

        return l_Handle;
    }
}