#include <Trinity/Platform/Backends/SDL3/SDLPlatform.h>

#include <SDL3/SDL.h>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Assert.h>
#include <Trinity/Platform/Backends/SDL3/SDLWindow.h>
#include <Trinity/Platform/Backends/SDL3/SDLInput.h>
#include <Trinity/Platform/Backends/SDL3/SDLGamepad.h>
#include <Trinity/Platform/Backends/SDL3/SDLFileSystem.h>
#include <Trinity/Platform/Events/ApplicationEvent.h>
#include <Trinity/Platform/Events/KeyEvent.h>
#include <Trinity/Platform/Events/MouseEvent.h>

namespace Trinity
{
    SDLPlatform::SDLPlatform()
    {
#if defined(TRINITY_PLATFORM_WINDOWS)
        m_Type = PlatformType::Windows;
#elif defined(TRINITY_PLATFORM_LINUX)
        m_Type = PlatformType::Linux;
#elif defined(TRINITY_PLATFORM_MACOS)
        m_Type = PlatformType::MacOS;
#else
        m_Type = PlatformType::Unknown;
#endif
    }

    SDLPlatform::~SDLPlatform()
    {
        Shutdown();
    }

    bool SDLPlatform::Initialize()
    {
        TR_CORE_ASSERT(!m_Initialized, "SDLPlatform already initialized");

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            TR_CORE_CRITICAL("SDLPlatform: SDL_Init failed: {}", SDL_GetError());
            return false;
        }

        m_FileSystem = std::make_unique<SDLFileSystem>();
        m_Input = std::make_unique<SDLInput>();
        m_Gamepad = std::make_unique<SDLGamepad>();

        m_Initialized = true;

        TR_CORE_INFO("SDLPlatform: initialized");
        return true;
    }

    void SDLPlatform::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        m_Window.reset();
        m_Gamepad.reset();
        m_Input.reset();
        m_FileSystem.reset();

        SDL_Quit();

        m_Initialized = false;

        TR_CORE_INFO("SDLPlatform: shut down");
    }

    Window& SDLPlatform::CreateWindow(const WindowProperties& properties)
    {
        TR_CORE_ASSERT(m_Initialized, "SDLPlatform not initialized");
        TR_CORE_ASSERT(m_Window == nullptr, "SDLPlatform window already created");

        m_Window = std::make_unique<SDLWindow>(properties);
        return *m_Window;
    }

    Window& SDLPlatform::GetWindow()
    {
        TR_CORE_ASSERT(m_Window != nullptr, "SDLPlatform has no window");
        return *m_Window;
    }

    bool SDLPlatform::HasWindow() const
    {
        return m_Window != nullptr;
    }

    Input& SDLPlatform::GetInput()
    {
        return *m_Input;
    }

    Gamepad& SDLPlatform::GetGamepad()
    {
        return *m_Gamepad;
    }

    FileSystem& SDLPlatform::GetFileSystem()
    {
        return *m_FileSystem;
    }

    void SDLPlatform::PollEvents()
    {
        SDLWindow* l_Window = static_cast<SDLWindow*>(m_Window.get());
        SDLGamepad* l_Gamepad = static_cast<SDLGamepad*>(m_Gamepad.get());

        SDL_Event l_Event;
        while (SDL_PollEvent(&l_Event))
        {
            switch (l_Event.type)
            {
                case SDL_EVENT_QUIT:
                {
                    if (l_Window != nullptr)
                    {
                        WindowCloseEvent l_Closed;
                        l_Window->DispatchEvent(l_Closed);
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                {
                    if (l_Window != nullptr && l_Event.window.windowID == l_Window->GetWindowID())
                    {
                        WindowCloseEvent l_Closed;
                        l_Window->DispatchEvent(l_Closed);
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_RESIZED:
                {
                    if (l_Window != nullptr && l_Event.window.windowID == l_Window->GetWindowID())
                    {
                        uint32_t l_NewWidth = static_cast<uint32_t>(l_Event.window.data1);
                        uint32_t l_NewHeight = static_cast<uint32_t>(l_Event.window.data2);
                        l_Window->SetSize(l_NewWidth, l_NewHeight);

                        WindowResizeEvent l_Resized(l_NewWidth, l_NewHeight);
                        l_Window->DispatchEvent(l_Resized);
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_MINIMIZED:
                {
                    if (l_Window != nullptr && l_Event.window.windowID == l_Window->GetWindowID())
                    {
                        l_Window->SetMinimized(true);
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_RESTORED:
                {
                    if (l_Window != nullptr && l_Event.window.windowID == l_Window->GetWindowID())
                    {
                        l_Window->SetMinimized(false);
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                {
                    if (l_Window != nullptr && l_Event.window.windowID == l_Window->GetWindowID())
                    {
                        WindowFocusEvent l_Focus;
                        l_Window->DispatchEvent(l_Focus);
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_FOCUS_LOST:
                {
                    if (l_Window != nullptr && l_Event.window.windowID == l_Window->GetWindowID())
                    {
                        WindowLostFocusEvent l_LostFocus;
                        l_Window->DispatchEvent(l_LostFocus);
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_MOVED:
                {
                    if (l_Window != nullptr && l_Event.window.windowID == l_Window->GetWindowID())
                    {
                        WindowMovedEvent l_Moved(l_Event.window.data1, l_Event.window.data2);
                        l_Window->DispatchEvent(l_Moved);
                    }
                    break;
                }

                case SDL_EVENT_KEY_DOWN:
                {
                    if (l_Window != nullptr)
                    {
                        KeyPressedEvent l_Pressed(static_cast<KeyCode>(l_Event.key.scancode), l_Event.key.repeat);
                        l_Window->DispatchEvent(l_Pressed);
                    }
                    break;
                }

                case SDL_EVENT_KEY_UP:
                {
                    if (l_Window != nullptr)
                    {
                        KeyReleasedEvent l_Released(static_cast<KeyCode>(l_Event.key.scancode));
                        l_Window->DispatchEvent(l_Released);
                    }
                    break;
                }

                case SDL_EVENT_TEXT_INPUT:
                {
                    if (l_Window != nullptr && l_Event.text.text != nullptr)
                    {
                        KeyTypedEvent l_Typed(static_cast<KeyCode>(l_Event.text.text[0]));
                        l_Window->DispatchEvent(l_Typed);
                    }
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    if (l_Window != nullptr)
                    {
                        MouseButtonPressedEvent l_Pressed(static_cast<MouseCode>(l_Event.button.button - 1));
                        l_Window->DispatchEvent(l_Pressed);
                    }
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    if (l_Window != nullptr)
                    {
                        MouseButtonReleasedEvent l_Released(static_cast<MouseCode>(l_Event.button.button - 1));
                        l_Window->DispatchEvent(l_Released);
                    }
                    break;
                }

                case SDL_EVENT_MOUSE_MOTION:
                {
                    if (l_Window != nullptr)
                    {
                        MouseMovedEvent l_Moved(l_Event.motion.x, l_Event.motion.y);
                        l_Window->DispatchEvent(l_Moved);
                    }
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL:
                {
                    if (l_Window != nullptr)
                    {
                        MouseScrolledEvent l_Scrolled(l_Event.wheel.x, l_Event.wheel.y);
                        l_Window->DispatchEvent(l_Scrolled);
                    }
                    break;
                }

                case SDL_EVENT_GAMEPAD_ADDED:
                {
                    if (l_Gamepad != nullptr)
                    {
                        l_Gamepad->HandleDeviceAdded(l_Event.gdevice.which);
                    }
                    break;
                }

                case SDL_EVENT_GAMEPAD_REMOVED:
                {
                    if (l_Gamepad != nullptr)
                    {
                        l_Gamepad->HandleDeviceRemoved(l_Event.gdevice.which);
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
}