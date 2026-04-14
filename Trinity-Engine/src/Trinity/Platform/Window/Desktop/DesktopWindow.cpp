#include "Trinity/Platform/Window/Desktop/DesktopWindow.h"

#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/GamepadEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"
#include "Trinity/Platform/Input/Desktop/SDLEventTranslator.h"
#include "Trinity/Utilities/Log.h"

#include <memory>

namespace Trinity
{
    DesktopWindow::DesktopWindow()
    {

    }

    DesktopWindow::~DesktopWindow()
    {

    }

    void DesktopWindow::Initialize(const WindowProperties& properties)
    {
        if (m_Initialized)
        {
            TR_CORE_WARN("DesktopWindow::Initialize called more than once. Ignoring.");

            return;
        }

        if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            TR_CORE_CRITICAL("SDL_InitSubSystem failed: {}", SDL_GetError());

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

            SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

            return;
        }

        m_WindowID = SDL_GetWindowID(m_WindowHandle);

        m_ShouldClose = false;
        m_CursorVisible = true;
        m_CursorLocked = false;
        m_Initialized = true;
    }

    void DesktopWindow::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        SetCursorLocked(false);
        SetCursorVisible(true);

        for (auto& it_Gamepad : m_Gamepads)
        {
            if (it_Gamepad.second != nullptr)
            {
                SDL_CloseGamepad(it_Gamepad.second);
            }
        }
        m_Gamepads.clear();

        if (m_WindowHandle != nullptr)
        {
            SDL_DestroyWindow(m_WindowHandle);
            m_WindowHandle = nullptr;
        }

        m_WindowID = 0;
        m_Initialized = false;

        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    }

    void DesktopWindow::OnUpdate()
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

            if (l_Event.type == SDL_EVENT_GAMEPAD_ADDED)
            {
                SDL_Gamepad* l_Gamepad = SDL_OpenGamepad(l_Event.gdevice.which);
                if (l_Gamepad != nullptr)
                {
                    const int l_GamepadID = static_cast<int>(l_Event.gdevice.which);
                    auto a_GamePad = m_Gamepads.find(l_GamepadID);
                    if (a_GamePad != m_Gamepads.end())
                    {
                        if (a_GamePad->second != nullptr)
                        {
                            SDL_CloseGamepad(a_GamePad->second);
                        }
                    }

                    const char* l_Name = SDL_GetGamepadName(l_Gamepad);
                    m_EventQueue.PushEvent(std::make_unique<GamepadConnectedEvent>(l_GamepadID, l_Name != nullptr ? l_Name : "", true));
                }

                continue;
            }

            if (l_Event.type == SDL_EVENT_GAMEPAD_REMOVED)
            {
                const int l_GamepadID = static_cast<int>(l_Event.gdevice.which);
                auto a_GamePad = m_Gamepads.find(l_GamepadID);
                if (a_GamePad != m_Gamepads.end())
                {
                    if (a_GamePad->second != nullptr)
                    {
                        SDL_CloseGamepad(a_GamePad->second);
                    }
                    m_Gamepads.erase(a_GamePad);
                }

                m_EventQueue.PushEvent(std::make_unique<GamepadDisconnectedEvent>(l_GamepadID));

                continue;
            }

            if (l_Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            {
                const int l_GamepadID = static_cast<int>(l_Event.gbutton.which);
                const Code::GamepadButton l_Button = TranslateGamepadButton(static_cast<SDL_GamepadButton>(l_Event.gbutton.button));
                m_EventQueue.PushEvent(std::make_unique<GamepadButtonPressedEvent>(l_GamepadID, l_Button));

                continue;
            }

            if (l_Event.type == SDL_EVENT_GAMEPAD_BUTTON_UP)
            {
                const int l_GamepadID = static_cast<int>(l_Event.gbutton.which);
                const Code::GamepadButton l_Button = TranslateGamepadButton(static_cast<SDL_GamepadButton>(l_Event.gbutton.button));
                m_EventQueue.PushEvent(std::make_unique<GamepadButtonReleasedEvent>(l_GamepadID, l_Button));

                continue;
            }

            if (l_Event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
            {
                const int l_GamepadID = static_cast<int>(l_Event.gaxis.which);
                const Code::GamepadAxis l_Axis = TranslateGamepadAxis(static_cast<SDL_GamepadAxis>(l_Event.gaxis.axis));
                const float l_Value = l_Event.gaxis.value / 32767.0f;
                m_EventQueue.PushEvent(std::make_unique<GamepadAxisMovedEvent>(l_GamepadID, l_Axis, l_Value));

                continue;
            }
        }
    }

    void DesktopWindow::OnEvent(Event& e)
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

    void DesktopWindow::SetVSync(bool enabled)
    {
        m_Data.VSync = enabled;
    }

    void DesktopWindow::SetCursorVisible(bool visible)
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

    void DesktopWindow::SetCursorLocked(bool locked)
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

    NativeWindowHandle DesktopWindow::GetNativeHandle() const
    {
        NativeWindowHandle l_Handle{};
        l_Handle.Window = m_WindowHandle;
        l_Handle.Properties = SDL_GetWindowProperties(m_WindowHandle);

        return l_Handle;
    }
}