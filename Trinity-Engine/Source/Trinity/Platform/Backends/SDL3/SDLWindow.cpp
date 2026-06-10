#include <Trinity/Platform/Backends/SDL3/SDLWindow.h>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_mouse.h>

#include <Trinity/Platform/Events/ApplicationEvent.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static SDL_HitTestResult SDLCALL TrinityHitTest(SDL_Window* window, const SDL_Point* area, void* data)
    {
        SDLWindow* l_Self = static_cast<SDLWindow*>(data);

        int l_Width = 0;
        int l_Height = 0;
        SDL_GetWindowSize(window, &l_Width, &l_Height);

        const bool l_Maximized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;
        const int l_Border = 6;

        if (!l_Maximized)
        {
            bool l_Left = area->x < l_Border;
            bool l_Right = area->x >= l_Width - l_Border;
            bool l_Top = area->y < l_Border;
            bool l_Bottom = area->y >= l_Height - l_Border;

            if (l_Top && l_Left)
            {
                return SDL_HITTEST_RESIZE_TOPLEFT;
            }

            if (l_Top && l_Right)
            {
                return SDL_HITTEST_RESIZE_TOPRIGHT;
            }

            if (l_Bottom && l_Left)
            {
                return SDL_HITTEST_RESIZE_BOTTOMLEFT;
            }

            if (l_Bottom && l_Right)
            {
                return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
            }

            if (l_Left)
            {
                return SDL_HITTEST_RESIZE_LEFT;
            }

            if (l_Right)
            {
                return SDL_HITTEST_RESIZE_RIGHT;
            }

            if (l_Top)
            {
                return SDL_HITTEST_RESIZE_TOP;
            }

            if (l_Bottom)
            {
                return SDL_HITTEST_RESIZE_BOTTOM;
            }
        }

        int l_RegionX = 0;
        int l_RegionY = 0;
        int l_RegionWidth = 0;
        int l_RegionHeight = 0;
        l_Self->TitleBarHitRegion(l_RegionX, l_RegionY, l_RegionWidth, l_RegionHeight);

        if (l_RegionWidth > 0 && l_RegionHeight > 0 && area->x >= l_RegionX && area->x < l_RegionX + l_RegionWidth && area->y >= l_RegionY && area->y < l_RegionY + l_RegionHeight)
        {
            return SDL_HITTEST_DRAGGABLE;
        }

        return SDL_HITTEST_NORMAL;
    }

    SDLWindow::SDLWindow(const WindowProperties& properties)
    {
        m_Data.Title = properties.Title;
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Height;
        m_Data.VSync = properties.VSync;
        m_Data.CustomTitleBar = properties.CustomTitleBar;

        SDL_WindowFlags l_Flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (properties.Resizable)
        {
            l_Flags |= SDL_WINDOW_RESIZABLE;
        }

        if (properties.CustomTitleBar)
        {
            l_Flags |= SDL_WINDOW_BORDERLESS;
        }

        m_Window = SDL_CreateWindow(m_Data.Title.c_str(), static_cast<int>(m_Data.Width), static_cast<int>(m_Data.Height), l_Flags);
        if (m_Window == nullptr)
        {
            TR_CORE_CRITICAL("SDLWindow: SDL_CreateWindow failed: {}", SDL_GetError());
            return;
        }

        if (properties.CustomTitleBar)
        {
            SDL_SetWindowHitTest(m_Window, TrinityHitTest, this);
        }

        TR_CORE_INFO("SDLWindow: created '{}' ({}x{}){}", m_Data.Title, m_Data.Width, m_Data.Height, m_Data.CustomTitleBar ? " [custom title bar]" : "");
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

    void SDLWindow::SetRelativeMouseMode(bool enabled)
    {
        if (m_Window != nullptr)
        {
            SDL_SetWindowRelativeMouseMode(m_Window, enabled);
        }
    }

    void SDLWindow::Minimize()
    {
        if (m_Window != nullptr)
        {
            SDL_MinimizeWindow(m_Window);
        }
    }

    void SDLWindow::Maximize()
    {
        if (m_Window != nullptr)
        {
            SDL_MaximizeWindow(m_Window);
        }
    }

    void SDLWindow::Restore()
    {
        if (m_Window != nullptr)
        {
            SDL_RestoreWindow(m_Window);
        }
    }

    void SDLWindow::RequestClose()
    {
        WindowCloseEvent l_Closed;
        DispatchEvent(l_Closed);
    }

    bool SDLWindow::IsMaximized() const
    {
        return m_Window != nullptr && (SDL_GetWindowFlags(m_Window) & SDL_WINDOW_MAXIMIZED) != 0;
    }

    void SDLWindow::SetTitleBarHitRegion(int x, int y, int width, int height)
    {
        m_Data.TitleBarX = x;
        m_Data.TitleBarY = y;
        m_Data.TitleBarWidth = width;
        m_Data.TitleBarHeight = height;
    }

    void SDLWindow::TitleBarHitRegion(int& x, int& y, int& width, int& height) const
    {
        x = m_Data.TitleBarX;
        y = m_Data.TitleBarY;
        width = m_Data.TitleBarWidth;
        height = m_Data.TitleBarHeight;
    }

    float SDLWindow::GetContentScale() const
    {
        if (m_Window == nullptr)
        {
            return 1.0f;
        }

        float l_Scale = SDL_GetWindowDisplayScale(m_Window);

        return l_Scale > 0.0f ? l_Scale : 1.0f;
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