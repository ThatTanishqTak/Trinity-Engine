#include "Engine/Platform/GLFW/GLFWWindow.h"
#include "Engine/Utilities/Utilities.h"

#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/GamepadEvent.h"
#include "Engine/Input/InputCodes.h"

#include <GLFW/glfw3.h>

#include <cmath>

namespace Engine
{
    static bool s_GLFWInitialized = false;
    static uint32_t s_GLFWWindowCount = 0;

    static KeyCode ToKeyCode(int key)
    {
        return static_cast<KeyCode>(key);
    }

    static MouseCode ToMouseCode(int button)
    {
        return static_cast<MouseCode>(button);
    }

    static GamepadCode ToGamepadCode(int control)
    {
        return static_cast<GamepadCode>(control);
    }

    static void GLFWErrorCallback(int error, const char* description)
    {
        TR_CORE_ERROR("GLFW Error ({0}): {1}", error, description ? description : "Unknown");
    }

    //-------------------------------------------------------------------------------------------------------//

    GLFWWindow::GLFWWindow(const WindowProperties& properties)
    {
        Initialize(properties);
    }

    GLFWWindow::~GLFWWindow()
    {
        Shutdown();
    }

    void GLFWWindow::Initialize(const WindowProperties& properties)
    {
        m_Data.Title = properties.Title;
        m_Data.Width = properties.Width;
        m_Data.Height = properties.Height;
        m_Data.VSync = properties.VSync;

        if (!s_GLFWInitialized)
        {
            glfwSetErrorCallback(GLFWErrorCallback);

            if (glfwInit() != GLFW_TRUE)
            {
                TR_CORE_ERROR("Failed to initialize GLFW.");

                return;
            }

            s_GLFWInitialized = true;
        }

        // Vulkan-friendly window: no OpenGL context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
        if (!m_Window)
        {
            TR_CORE_ERROR("Failed to create GLFW window.");

            return;
        }

        s_GLFWWindowCount++;

        glfwSetWindowUserPointer(m_Window, &m_Data);

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
        {
            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            WindowCloseEvent l_Event;
            a_Data.EventCallback(l_Event);
        });

        glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
        {
            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            a_Data.Width = (uint32_t)width;
            a_Data.Height = (uint32_t)height;
            a_Data.FramebufferResized = true;

            if (!a_Data.EventCallback)
            {
                return;
            }

            WindowResizeEvent l_Event((uint32_t)width, (uint32_t)height);
            a_Data.EventCallback(l_Event);
        });

        glfwSetWindowFocusCallback(m_Window, [](GLFWwindow* window, int focused)
        {
            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            if (focused)
            {
                WindowFocusEvent l_Event;
                a_Data.EventCallback(l_Event);
            }
            else
            {
                WindowLostFocusEvent l_Event;
                a_Data.EventCallback(l_Event);
            }
        });

        glfwSetWindowPosCallback(m_Window, [](GLFWwindow* window, int x, int y)
        {
            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            WindowMovedEvent l_Event(x, y);
            a_Data.EventCallback(l_Event);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            (void)scancode;
            (void)mods;

            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            switch (action)
            {
                case GLFW_PRESS:
                {
                    const KeyCode l_KeyCode = ToKeyCode(key);
                    KeyPressedEvent l_Event(l_KeyCode, 0);
                    a_Data.EventCallback(l_Event);

                    break;
                }
                case GLFW_RELEASE:
                {
                    const KeyCode l_KeyCode = ToKeyCode(key);
                    KeyReleasedEvent l_Event(l_KeyCode);
                    a_Data.EventCallback(l_Event);

                    break;
                }
                case GLFW_REPEAT:
                {
                    const KeyCode l_KeyCode = ToKeyCode(key);
                    KeyPressedEvent l_Event(l_KeyCode, 1);
                    a_Data.EventCallback(l_Event);

                    break;
                }
                default:
                    break;
            }
        });

        glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint)
        {
            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            const KeyCode l_KeyCode = ToKeyCode((int)codepoint);
            KeyTypedEvent l_Event(l_KeyCode);
            a_Data.EventCallback(l_Event);
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
        {
            (void)mods;

            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            switch (action)
            {
                case GLFW_PRESS:
                {
                    const MouseCode l_ButtonCode = ToMouseCode(button);
                    MouseButtonPressedEvent l_Event(l_ButtonCode);
                    a_Data.EventCallback(l_Event);

                    break;
                }
                case GLFW_RELEASE:
                {
                    const MouseCode l_ButtonCode = ToMouseCode(button);
                    MouseButtonReleasedEvent l_Event(l_ButtonCode);
                    a_Data.EventCallback(l_Event);

                    break;
                }
                default:
                    break;
            }
        });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            MouseScrolledEvent l_Event((float)xOffset, (float)yOffset);
            a_Data.EventCallback(l_Event);
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
        {
            auto& a_Data = *static_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(window));

            if (!a_Data.EventCallback)
            {
                return;
            }

            MouseMovedEvent l_Event((float)x, (float)y);
            a_Data.EventCallback(l_Event);
        });

        TR_CORE_INFO("Window created: \"{0}\" ({1}x{2})", m_Data.Title, m_Data.Width, m_Data.Height);
    }

    void GLFWWindow::Shutdown()
    {
        if (m_Window)
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
            s_GLFWWindowCount--;

            if (s_GLFWWindowCount == 0 && s_GLFWInitialized)
            {
                glfwTerminate();
                s_GLFWInitialized = false;
            }
        }
    }

    void GLFWWindow::OnUpdate()
    {
        glfwPollEvents();

        UpdateGamepads();
    }

    void GLFWWindow::SetVSync(bool enabled)
    {
        m_Data.VSync = enabled;
    }

    bool GLFWWindow::ShouldClose() const
    {
        if (!m_Window)
        {
            return true;
        }

        return glfwWindowShouldClose(m_Window) == GLFW_TRUE;
    }

    void GLFWWindow::UpdateGamepads()
    {
        if (!m_Data.EventCallback)
        {
            return;
        }

        constexpr float l_Deadzone = 0.08f;
        constexpr float l_Epsilon = 0.01f;

        for (int it_GamepadID = 0; it_GamepadID < s_MaxGamepads; ++it_GamepadID)
        {
            const bool l_Present = (glfwJoystickPresent(it_GamepadID) == GLFW_TRUE);
            GamepadData& l_GamepadData = m_Gamepads[(size_t)it_GamepadID];

            // Connect / disconnect
            if (l_Present != l_GamepadData.Present)
            {
                l_GamepadData.Present = l_Present;
                l_GamepadData.HasState = false;

                if (l_Present)
                {
                    const char* l_Name = glfwGetJoystickName(it_GamepadID);
                    const bool l_Mapped = (glfwJoystickIsGamepad(it_GamepadID) == GLFW_TRUE);

                    GamepadConnectedEvent l_Event(it_GamepadID, l_Name ? l_Name : "", l_Mapped);
                    m_Data.EventCallback(l_Event);
                }
                else
                {
                    GamepadDisconnectedEvent l_Event(it_GamepadID);
                    m_Data.EventCallback(l_Event);
                }

                continue;
            }

            if (!l_Present)
            {
                continue;
            }

            if (glfwJoystickIsGamepad(it_GamepadID) != GLFW_TRUE)
            {
                continue;
            }

            GLFWgamepadstate l_State{};
            if (glfwGetGamepadState(it_GamepadID, &l_State) != GLFW_TRUE)
            {
                continue;
            }

            // Buttons
            for (int it_Button = 0; it_Button < s_MaxButtons; ++it_Button)
            {
                const unsigned char l_NewValue = l_State.buttons[it_Button];
                const unsigned char l_OldValue = l_GamepadData.HasState ? l_GamepadData.Buttons[(size_t)it_Button] : l_NewValue;

                if (l_GamepadData.HasState && l_NewValue != l_OldValue)
                {
                    if (l_NewValue == GLFW_PRESS)
                    {
                        const GamepadCode l_ButtonCode = ToGamepadCode(it_Button);
                        GamepadButtonPressedEvent l_Event(it_GamepadID, l_ButtonCode);
                        m_Data.EventCallback(l_Event);
                    }
                    else
                    {
                        const GamepadCode l_ButtonCode = ToGamepadCode(it_Button);
                        GamepadButtonReleasedEvent l_Event(it_GamepadID, l_ButtonCode);
                        m_Data.EventCallback(l_Event);
                    }
                }

                l_GamepadData.Buttons[(size_t)it_Button] = l_NewValue;
            }

            // Axes
            for (int it_Axis = 0; it_Axis < s_MaxAxes; ++it_Axis)
            {
                float l_Value = l_State.axes[it_Axis];

                // Deadzone only for sticks (0..3). Triggers (4..5) usually don't need it.
                if (it_Axis <= 3 && std::fabs(l_Value) < l_Deadzone)
                {
                    l_Value = 0.0f;
                }

                const float l_OldValue = l_GamepadData.HasState ? l_GamepadData.Axes[(size_t)it_Axis] : l_Value;

                if (l_GamepadData.HasState && std::fabs(l_Value - l_OldValue) > l_Epsilon)
                {
                    const GamepadCode l_AxisCode = ToGamepadCode(it_Axis);
                    GamepadAxisMovedEvent l_Event(it_GamepadID, l_AxisCode, l_Value);
                    m_Data.EventCallback(l_Event);
                }

                l_GamepadData.Axes[(size_t)it_Axis] = l_Value;
            }

            l_GamepadData.HasState = true;
        }
    }
}