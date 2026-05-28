#include <Trinity/Platform/Backends/SDL3/SDLWindow.h>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_properties.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    SDLWindow::SDLWindow(const WindowProperties& properties)
    {
        m_Data.Title = properties.Title;
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Height;
        m_Data.VSync = properties.VSync;

        SDL_WindowFlags l_Flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (properties.Resizable)
        {
            l_Flags |= SDL_WINDOW_RESIZABLE;
        }

        m_Window = SDL_CreateWindow(m_Data.Title.c_str(), static_cast<int>(m_Data.Width), static_cast<int>(m_Data.Height), l_Flags);
        if (m_Window == nullptr)
        {
            TR_CORE_CRITICAL("SDLWindow: SDL_CreateWindow failed: {}", SDL_GetError());
            return;
        }

        TR_CORE_INFO("SDLWindow: created '{}' ({}x{})", m_Data.Title, m_Data.Width, m_Data.Height);
    }

    SDLWindow::~SDLWindow()
    {
        if (m_Window != nullptr)
        {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void SDLWindow::OnUpdate()
    {
    }

    void SDLWindow::SetTitle(const std::string& title)
    {
        m_Data.Title = title;
        if (m_Window != nullptr)
        {
            SDL_SetWindowTitle(m_Window, title.c_str());
        }
    }

    void SDLWindow::SetVSync(bool enabled)
    {
        m_Data.VSync = enabled;
    }

    uint32_t SDLWindow::GetWindowID() const
    {
        if (m_Window == nullptr)
        {
            return 0;
        }

        return static_cast<uint32_t>(SDL_GetWindowID(m_Window));
    }

    void SDLWindow::DispatchEvent(Event& event)
    {
        if (m_Data.Callback)
        {
            m_Data.Callback(event);
        }
    }

    void SDLWindow::SetSize(uint32_t width, uint32_t height)
    {
        m_Data.Width = width;
        m_Data.Height = height;
    }

    void SDLWindow::SetMinimized(bool minimized)
    {
        m_Data.Minimized = minimized;
    }

    NativeWindowHandle SDLWindow::GetNativeHandle() const
    {
        NativeWindowHandle l_Handle;
        if (m_Window == nullptr)
        {
            return l_Handle;
        }

        SDL_PropertiesID l_Props = SDL_GetWindowProperties(m_Window);

#if defined(TRINITY_PLATFORM_WINDOWS)
        l_Handle.Type = NativeHandleType::Win32;
        l_Handle.Handle = SDL_GetPointerProperty(l_Props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        l_Handle.Display = nullptr;
#elif defined(TRINITY_PLATFORM_LINUX)
        void* l_WaylandSurface = SDL_GetPointerProperty(l_Props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        if (l_WaylandSurface != nullptr)
        {
            l_Handle.Type = NativeHandleType::Wayland;
            l_Handle.Handle = l_WaylandSurface;
            l_Handle.Display = SDL_GetPointerProperty(l_Props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        }
        else
        {
            l_Handle.Type = NativeHandleType::Xlib;
            l_Handle.Handle = reinterpret_cast<void*>(static_cast<uintptr_t>(SDL_GetNumberProperty(l_Props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0)));
            l_Handle.Display = SDL_GetPointerProperty(l_Props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        }
#elif defined(TRINITY_PLATFORM_MACOS)
        l_Handle.Type = NativeHandleType::Cocoa;
        l_Handle.Handle = SDL_GetPointerProperty(l_Props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
        l_Handle.Display = nullptr;
#endif

        return l_Handle;
    }
}