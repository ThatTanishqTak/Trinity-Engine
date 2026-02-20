#include "Trinity/Platform/Windows/WindowsWindow.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>

#include <memory>
#include <cstddef>
#include <cstdlib>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Trinity
{
    static const char* s_WindowClassName = "TrinityWindowClass";
    static ATOM s_WindowClassAtom = 0;
    static uint32_t s_WindowCount = 0;

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

        TR_CORE_TRACE("Creating Win32 Window");

        m_InstanceHandle = GetModuleHandleA(nullptr);

        m_Data.Title = properties.Title;
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Height;
        m_Data.VSync = properties.VSync;
        m_Data.Resizable = properties.Resizable;
        m_Data.Minimized = (properties.Width == 0 || properties.Height == 0);

        RegisterWindowClassIfNeeded();
        CreateNativeWindow();

        RAWINPUTDEVICE l_MouseRawInputDevice{};
        l_MouseRawInputDevice.usUsagePage = 0x01;
        l_MouseRawInputDevice.usUsage = 0x02;
        l_MouseRawInputDevice.dwFlags = 0;
        l_MouseRawInputDevice.hwndTarget = m_WindowHandle;

        if (!RegisterRawInputDevices(&l_MouseRawInputDevice, 1, sizeof(RAWINPUTDEVICE)))
        {
            TR_CORE_WARN("RegisterRawInputDevices failed.");
        }

        m_ShouldClose = false;
        m_CursorVisible = true;
        m_CursorLocked = false;
        m_Initialized = true;

        TR_CORE_TRACE("Title: {}", m_Data.Title);
        TR_CORE_TRACE("Resolution: {}x{}", m_Data.Width, m_Data.Height);
        TR_CORE_TRACE("Win32 Window Created");
    }

    void WindowsWindow::Shutdown()
    {
        TR_CORE_TRACE("Shuting Down Win32 Window");

        if (!m_Initialized)
        {
            return;
        }

        SetCursorLocked(false);
        SetCursorVisible(true);
        DestroyNativeWindow();
        m_Initialized = false;

        TR_CORE_TRACE("Win32 Window Shutdown Complete");
    }

    void WindowsWindow::OnUpdate()
    {
        if (!m_Initialized)
        {
            return;
        }

        MSG l_Message{};
        while (PeekMessageA(&l_Message, nullptr, 0, 0, PM_REMOVE))
        {
            if (l_Message.message == WM_QUIT)
            {
                m_ShouldClose = true;

                break;
            }

            TranslateMessage(&l_Message);
            DispatchMessageA(&l_Message);
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
        // Vulkan VSync is a swapchain present-mode choice. This flag is stored for later.
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
            int l_ShowCount = ShowCursor(TRUE);
            while (l_ShowCount < 0)
            {
                l_ShowCount = ShowCursor(TRUE);
            }
        }
        else
        {
            int l_ShowCount = ShowCursor(FALSE);
            while (l_ShowCount >= 0)
            {
                l_ShowCount = ShowCursor(FALSE);
            }
        }
    }

    void WindowsWindow::SetCursorLocked(bool locked)
    {
        if (m_CursorLocked == locked)
            return;

        // Save cursor position on lock so it can be restored on unlock.
        if (locked)
        {
            m_HasCursorRestorePos = (GetCursorPos(&m_CursorRestorePos) != 0);
            if (m_WindowHandle)
            {
                SetCapture(m_WindowHandle);
            }
        }

        m_CursorLocked = locked;
        ApplyCursorLockClip();

        if (!locked)
        {
            ReleaseCapture();

            if (m_HasCursorRestorePos)
            {
                SetCursorPos(m_CursorRestorePos.x, m_CursorRestorePos.y);
                m_HasCursorRestorePos = false;
            }
        }
    }

    NativeWindowHandle WindowsWindow::GetNativeHandle() const
    {
        NativeWindowHandle l_Handle{};
        l_Handle.WindowType = NativeWindowHandle::Type::Win32;
        l_Handle.Handle1 = reinterpret_cast<void*>(m_WindowHandle);
        l_Handle.Handle2 = reinterpret_cast<void*>(m_InstanceHandle);

        return l_Handle;
    }

    void WindowsWindow::RegisterWindowClassIfNeeded()
    {
        if (s_WindowClassAtom != 0)
        {
            return;
        }

        WNDCLASSEXA l_Wc{};
        l_Wc.cbSize = sizeof(WNDCLASSEXA);
        l_Wc.style = CS_HREDRAW | CS_VREDRAW;
        l_Wc.lpfnWndProc = &WindowsWindow::WindowProc;
        l_Wc.cbClsExtra = 0;
        l_Wc.cbWndExtra = 0;
        l_Wc.hInstance = m_InstanceHandle;
        l_Wc.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
        l_Wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
        l_Wc.hbrBackground = nullptr;
        l_Wc.lpszMenuName = nullptr;
        l_Wc.lpszClassName = s_WindowClassName;
        l_Wc.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);

        s_WindowClassAtom = RegisterClassExA(&l_Wc);
        if (s_WindowClassAtom == 0)
        {
            TR_CORE_CRITICAL("RegisterClassExA failed.");

            std::abort();
        }
    }

    void WindowsWindow::CreateNativeWindow()
    {
        DWORD l_Style = WS_OVERLAPPEDWINDOW;
        if (!m_Data.Resizable)
        {
            l_Style &= ~WS_THICKFRAME;
            l_Style &= ~WS_MAXIMIZEBOX;
        }

        RECT l_Rect{};
        l_Rect.left = 0;
        l_Rect.top = 0;
        l_Rect.right = static_cast<LONG>(m_Data.Width);
        l_Rect.bottom = static_cast<LONG>(m_Data.Height);

        AdjustWindowRect(&l_Rect, l_Style, FALSE);

        const int l_WindowWidth = l_Rect.right - l_Rect.left;
        const int l_WindowHeight = l_Rect.bottom - l_Rect.top;

        const int l_ScreenX = (GetSystemMetrics(SM_CXSCREEN) - l_WindowWidth) / 2;
        const int l_ScreenY = (GetSystemMetrics(SM_CYSCREEN) - l_WindowHeight) / 2;

        m_WindowHandle = CreateWindowExA(0, s_WindowClassName, m_Data.Title.c_str(), l_Style, l_ScreenX, l_ScreenY, l_WindowWidth, l_WindowHeight, nullptr, nullptr,
            m_InstanceHandle, this);

        if (m_WindowHandle == nullptr)
        {
            TR_CORE_CRITICAL("CreateWindowExA failed.");

            std::abort();
        }

        ++s_WindowCount;

        ShowWindow(m_WindowHandle, SW_SHOW);
        UpdateWindow(m_WindowHandle);
    }

    void WindowsWindow::DestroyNativeWindow()
    {
        if (m_WindowHandle)
        {
            DestroyWindow(m_WindowHandle);
            m_WindowHandle = nullptr;

            if (s_WindowCount > 0)
            {
                --s_WindowCount;
            }
        }

        if (s_WindowCount == 0 && s_WindowClassAtom != 0)
        {
            UnregisterClassA(s_WindowClassName, m_InstanceHandle);
            s_WindowClassAtom = 0;
        }
    }

    void WindowsWindow::ApplyCursorLockClip()
    {
        if (!m_CursorLocked || m_WindowHandle == nullptr || GetForegroundWindow() != m_WindowHandle || m_Data.Minimized)
        {
            ClipCursor(nullptr);

            return;
        }

        RECT l_ClientRect{};
        if (!GetClientRect(m_WindowHandle, &l_ClientRect))
        {
            ClipCursor(nullptr);

            return;
        }

        POINT l_TopLeft{ l_ClientRect.left, l_ClientRect.top };
        POINT l_BottomRight{ l_ClientRect.right, l_ClientRect.bottom };

        if (!ClientToScreen(m_WindowHandle, &l_TopLeft) || !ClientToScreen(m_WindowHandle, &l_BottomRight))
        {
            ClipCursor(nullptr);

            return;
        }

        RECT l_ClipRect{};
        l_ClipRect.left = l_TopLeft.x;
        l_ClipRect.top = l_TopLeft.y;
        l_ClipRect.right = l_BottomRight.x;
        l_ClipRect.bottom = l_BottomRight.y;

        ClipCursor(&l_ClipRect);
    }

    LRESULT CALLBACK WindowsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        WindowsWindow* l_Window = nullptr;

        if (msg == WM_NCCREATE)
        {
            auto* a_Create = reinterpret_cast<CREATESTRUCTA*>(lParam);
            l_Window = reinterpret_cast<WindowsWindow*>(a_Create->lpCreateParams);

            SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(l_Window));
            l_Window->m_WindowHandle = hWnd;
        }
        else
        {
            l_Window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
        }

        if (l_Window)
        {
            return l_Window->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }

    LRESULT WindowsWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Let ImGui process the message first
        if (ImGui::GetCurrentContext() != nullptr)
        {
            if (ImGui_ImplWin32_WndProcHandler(m_WindowHandle, msg, wParam, lParam))
            {
                return true;
            }
        }

        switch (msg)
        {
            case WM_CLOSE:
            {
                m_EventQueue.PushEvent(std::make_unique<WindowCloseEvent>());

                return 0;
            }

            case WM_DESTROY:
            {
                PostQuitMessage(0);

                return 0;
            }

            case WM_SIZE:
            {
                const uint32_t l_Width = static_cast<uint32_t>(LOWORD(lParam));
                const uint32_t l_Height = static_cast<uint32_t>(HIWORD(lParam));
                m_Data.Minimized = (wParam == SIZE_MINIMIZED) || (l_Width == 0 || l_Height == 0);

                ApplyCursorLockClip();
                m_EventQueue.PushEvent(std::make_unique<WindowResizeEvent>(l_Width, l_Height));

                return 0;
            }

            case WM_SETFOCUS:
            {
                ApplyCursorLockClip();
                m_EventQueue.PushEvent(std::make_unique<WindowFocusEvent>());

                return 0;
            }

            case WM_KILLFOCUS:
            {
                if (m_CursorLocked)
                {
                    SetCursorLocked(false);
                    SetCursorVisible(true);
                }

                ApplyCursorLockClip();
                m_EventQueue.PushEvent(std::make_unique<WindowLostFocusEvent>());

                return 0;
            }

            case WM_MOVE:
            {
                const int l_X = static_cast<int>(static_cast<short>(LOWORD(lParam)));
                const int l_Y = static_cast<int>(static_cast<short>(HIWORD(lParam)));

                ApplyCursorLockClip();
                m_EventQueue.PushEvent(std::make_unique<WindowMovedEvent>(l_X, l_Y));

                return 0;
            }

            case WM_MOUSEMOVE:
            {
                if (m_CursorLocked)
                {
                    return 0;
                }

                const float l_X = static_cast<float>(static_cast<short>(LOWORD(lParam)));
                const float l_Y = static_cast<float>(static_cast<short>(HIWORD(lParam)));

                m_EventQueue.PushEvent(std::make_unique<MouseMovedEvent>(l_X, l_Y));

                return 0;
            }

            case WM_MOUSEWHEEL:
            {
                const int l_Delta = GET_WHEEL_DELTA_WPARAM(wParam);
                const float l_Steps = static_cast<float>(l_Delta) / static_cast<float>(WHEEL_DELTA);

                m_EventQueue.PushEvent(std::make_unique<MouseScrolledEvent>(0.0f, l_Steps));

                return 0;
            }

            case WM_INPUT:
            {
                UINT l_RawInputSize = 0;
                if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &l_RawInputSize, sizeof(RAWINPUTHEADER)) != 0 || l_RawInputSize == 0)
                {
                    return 0;
                }

                std::vector<std::byte> l_RawInputBuffer(l_RawInputSize);
                const UINT l_BytesCopied = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, l_RawInputBuffer.data(), &l_RawInputSize, sizeof(RAWINPUTHEADER));
                if (l_BytesCopied == static_cast<UINT>(-1) || l_BytesCopied != l_RawInputSize)
                {
                    return 0;
                }

                const RAWINPUT* a_RawInput = reinterpret_cast<const RAWINPUT*>(l_RawInputBuffer.data());
                if (a_RawInput->header.dwType == RIM_TYPEMOUSE)
                {
                    const float l_XDelta = static_cast<float>(a_RawInput->data.mouse.lLastX);
                    const float l_YDelta = static_cast<float>(a_RawInput->data.mouse.lLastY);
                    m_EventQueue.PushEvent(std::make_unique<MouseRawDeltaEvent>(l_XDelta, l_YDelta));
                }

                return 0;
            }

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
            {
                const Code::MouseCode l_Button = TranslateMouseButton(msg, wParam);
                m_EventQueue.PushEvent(std::make_unique<MouseButtonPressedEvent>(l_Button));

                return 0;
            }

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
            {
                const Code::MouseCode l_Button = TranslateMouseButton(msg, wParam);
                m_EventQueue.PushEvent(std::make_unique<MouseButtonReleasedEvent>(l_Button));

                return 0;
            }

            case WM_CHAR:
            {
                const uint32_t l_Codepoint = static_cast<uint32_t>(wParam);
                m_EventQueue.PushEvent(std::make_unique<KeyTypedEvent>(l_Codepoint));

                return 0;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                const int l_RepeatCount = static_cast<int>(LOWORD(lParam));

                const Code::KeyCode l_Key = TranslateKeyCode(wParam, lParam);
                m_EventQueue.PushEvent(std::make_unique<KeyPressedEvent>(l_Key, l_RepeatCount));

                return 0;
            }

            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const Code::KeyCode l_Key = TranslateKeyCode(wParam, lParam);
                m_EventQueue.PushEvent(std::make_unique<KeyReleasedEvent>(l_Key));

                return 0;
            }

            default:
                break;
        }

        return DefWindowProcA(m_WindowHandle, msg, wParam, lParam);
    }

    Code::MouseCode WindowsWindow::TranslateMouseButton(UINT msg, WPARAM wParam)
    {
        (void)msg;

        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) return Code::MouseCode::TR_BUTTON_LEFT;
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP) return Code::MouseCode::TR_BUTTON_RIGHT;
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) return Code::MouseCode::TR_BUTTON_MIDDLE;

        if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONUP)
        {
            const WORD l_X = GET_XBUTTON_WPARAM(wParam);

            return (l_X == XBUTTON1) ? Code::MouseCode::TR_BUTTON_3 : Code::MouseCode::TR_BUTTON_4;
        }

        return Code::MouseCode::TR_BUTTON_0;
    }

    Code::KeyCode WindowsWindow::TranslateKeyCode(WPARAM wParam, LPARAM lParam)
    {
        // Keep engine KeyCode values stable (currently GLFW-style values) and translate from Win32 VK codes.
        const UINT l_Vk = static_cast<UINT>(wParam);

        // Letters and digits map cleanly.
        if (l_Vk >= 'A' && l_Vk <= 'Z') return static_cast<Code::KeyCode>(l_Vk);
        if (l_Vk >= '0' && l_Vk <= '9') return static_cast<Code::KeyCode>(l_Vk);

        switch (l_Vk)
        {
            case VK_SPACE:      return Code::KeyCode::TR_KEY_SPACE;
            case VK_ESCAPE:     return Code::KeyCode::TR_KEY_ESCAPE;
            case VK_RETURN:     return Code::KeyCode::TR_KEY_ENTER;
            case VK_TAB:        return Code::KeyCode::TR_KEY_TAB;
            case VK_BACK:       return Code::KeyCode::TR_KEY_BACKSPACE;
            case VK_INSERT:     return Code::KeyCode::TR_KEY_INSERT;
            case VK_DELETE:     return Code::KeyCode::TR_KEY_DELETE;

            case VK_RIGHT:      return Code::KeyCode::TR_KEY_RIGHT;
            case VK_LEFT:       return Code::KeyCode::TR_KEY_LEFT;
            case VK_DOWN:       return Code::KeyCode::TR_KEY_DOWN;
            case VK_UP:         return Code::KeyCode::TR_KEY_UP;

            case VK_PRIOR:      return Code::KeyCode::TR_KEY_PAGE_UP;
            case VK_NEXT:       return Code::KeyCode::TR_KEY_PAGE_DOWN;
            case VK_HOME:       return Code::KeyCode::TR_KEY_HOME;
            case VK_END:        return Code::KeyCode::TR_KEY_END;

            case VK_CAPITAL:    return Code::KeyCode::TR_KEY_CAPSLOCK;
            case VK_SCROLL:     return Code::KeyCode::TR_KEY_SCROLLLOCK;
            case VK_NUMLOCK:    return Code::KeyCode::TR_KEY_NUMLOCK;
            case VK_SNAPSHOT:   return Code::KeyCode::TR_KEY_PRINTSCREEN;
            case VK_PAUSE:      return Code::KeyCode::TR_KEY_PAUSE;

            // OEM keys (layout-dependent, but VK codes are the best you get at this layer).
            case VK_OEM_3:      return Code::KeyCode::TR_KEY_GRAVE_ACCENT;
            case VK_OEM_MINUS:  return Code::KeyCode::TR_KEY_MINUS;
            case VK_OEM_PLUS:   return Code::KeyCode::TR_KEY_EQUAL;
            case VK_OEM_4:      return Code::KeyCode::TR_KEY_LEFT_BRACKET;
            case VK_OEM_6:      return Code::KeyCode::TR_KEY_RIGHT_BRACKET;
            case VK_OEM_5:      return Code::KeyCode::TR_KEY_BACK_SLASH;
            case VK_OEM_1:      return Code::KeyCode::TR_KEY_SEMICOLON;
            case VK_OEM_7:      return Code::KeyCode::TR_KEY_APOSTROPHE;
            case VK_OEM_COMMA:  return Code::KeyCode::TR_KEY_COMMA;
            case VK_OEM_PERIOD: return Code::KeyCode::TR_KEY_PERIOD;
            case VK_OEM_2:      return Code::KeyCode::TR_KEY_SLASH;

            // Function keys
            case VK_F1:  return Code::KeyCode::TR_KEY_F1;
            case VK_F2:  return Code::KeyCode::TR_KEY_F2;
            case VK_F3:  return Code::KeyCode::TR_KEY_F3;
            case VK_F4:  return Code::KeyCode::TR_KEY_F4;
            case VK_F5:  return Code::KeyCode::TR_KEY_F5;
            case VK_F6:  return Code::KeyCode::TR_KEY_F6;
            case VK_F7:  return Code::KeyCode::TR_KEY_F7;
            case VK_F8:  return Code::KeyCode::TR_KEY_F8;
            case VK_F9:  return Code::KeyCode::TR_KEY_F9;
            case VK_F10: return Code::KeyCode::TR_KEY_F10;
            case VK_F11: return Code::KeyCode::TR_KEY_F11;
            case VK_F12: return Code::KeyCode::TR_KEY_F12;

            // Keypad
            case VK_NUMPAD0: return Code::KeyCode::TR_KEY_KP0;
            case VK_NUMPAD1: return Code::KeyCode::TR_KEY_KP1;
            case VK_NUMPAD2: return Code::KeyCode::TR_KEY_KP2;
            case VK_NUMPAD3: return Code::KeyCode::TR_KEY_KP3;
            case VK_NUMPAD4: return Code::KeyCode::TR_KEY_KP4;
            case VK_NUMPAD5: return Code::KeyCode::TR_KEY_KP5;
            case VK_NUMPAD6: return Code::KeyCode::TR_KEY_KP6;
            case VK_NUMPAD7: return Code::KeyCode::TR_KEY_KP7;
            case VK_NUMPAD8: return Code::KeyCode::TR_KEY_KP8;
            case VK_NUMPAD9: return Code::KeyCode::TR_KEY_KP9;

            case VK_DECIMAL: return Code::KeyCode::TR_KEY_KEYPAD_DECIMAL;
            case VK_DIVIDE:  return Code::KeyCode::TR_KEY_KEYPAD_DIVIDE;
            case VK_MULTIPLY:return Code::KeyCode::TR_KEY_KEYPAD_MULTIPLY;
            case VK_SUBTRACT:return Code::KeyCode::TR_KEY_KEYPAD_SUBTRACT;
            case VK_ADD:     return Code::KeyCode::TR_KEY_KEYPAD_ADD;

            // Modifiers (left/right)
            case VK_SHIFT:
            {
                const UINT l_Scancode = (static_cast<UINT>(lParam) & 0x00FF0000) >> 16;
                const UINT l_VkEx = MapVirtualKeyA(l_Scancode, MAPVK_VSC_TO_VK_EX);
                return (l_VkEx == VK_RSHIFT) ? Code::KeyCode::TR_KEY_RIGHT_SHIFT : Code::KeyCode::TR_KEY_LEFT_SHIFT;
            }
            case VK_CONTROL:
            {
                const bool l_Extended = (lParam & (1 << 24)) != 0;
                return l_Extended ? Code::KeyCode::TR_KEY_RIGHT_CONTROL : Code::KeyCode::TR_KEY_LEFT_CONTROL;
            }
            case VK_MENU:
            {
                const bool l_Extended = (lParam & (1 << 24)) != 0;
                return l_Extended ? Code::KeyCode::TR_KEY_RIGHT_ALT : Code::KeyCode::TR_KEY_LEFT_ALT;
            }
            default:
                break;
        }

        return Code::KeyCode::UNKNOWN;
    }
}