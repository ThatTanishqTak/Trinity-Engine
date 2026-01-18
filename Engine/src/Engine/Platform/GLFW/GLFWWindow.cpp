#include "Engine/Platform/GLFW/GLFWWindow.h"
#include "Engine/Core/Utilities.h"

#include "Engine/Events/WindowEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/GamepadEvent.h"

#include "Engine/Events/Input/KeyCodes.h"
#include "Engine/Events/Input/MouseCodes.h"
#include "Engine/Events/Input/GamepadCodes.h"

#include <GLFW/glfw3.h>

static uint16_t ConvertGLFWMods(int mods)
{
    uint16_t l_Mod = Engine::Mod_None;
    if (mods & GLFW_MOD_SHIFT)
    {
        l_Mod |= Engine::Mod_Shift;
    }

    if (mods & GLFW_MOD_CONTROL)
    {
        l_Mod |= Engine::Mod_Control;
    }
    
    if (mods & GLFW_MOD_ALT)
    {
        l_Mod |= Engine::Mod_Alt;
    }
    
    if (mods & GLFW_MOD_SUPER)
    {
        l_Mod |= Engine::Mod_Super;
    }

    if (mods & GLFW_MOD_CAPS_LOCK)
    {
        l_Mod |= Engine::Mod_CapsLock;
    }

    if (mods & GLFW_MOD_NUM_LOCK)
    {
        l_Mod |= Engine::Mod_NumLock;
    }

    return l_Mod;
}

namespace Engine
{
    static bool s_GLFWInitialized = false;
    static uint32_t s_GLFWWindowCount = 0;

    static void GLFWErrorCallback(int error, const char* description)
    {
        TR_CORE_ERROR("GLFW Error ({0}): {1}", error, description ? description : "Unknown");
    }

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

        glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                a_Data.Width = (uint32_t)width;
                a_Data.Height = (uint32_t)height;
                a_Data.FramebufferResized = true;

                if (a_Data.EventCallback)
                {
                    Engine::WindowResizeEvent e((uint32_t)width, (uint32_t)height);
                    a_Data.EventCallback(e);
                }
            });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                if (a_Data.EventCallback)
                {
                    Engine::WindowCloseEvent e;
                    a_Data.EventCallback(e);
                }
            });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                if (a_Data.EventCallback)
                {
                    Engine::MouseMovedEvent e(x, y);
                    a_Data.EventCallback(e);
                }
            });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOff, double yOff)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                if (a_Data.EventCallback)
                {
                    Engine::MouseScrolledEvent e(xOff, yOff);
                    a_Data.EventCallback(e);
                }
            });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                if (!a_Data.EventCallback)
                {
                    return;
                }

                const uint16_t l_Mod = ConvertGLFWMods(mods);
                const auto l_TRButton = Engine::Input::FromGLFWMouseButton(button);

                if (action == GLFW_PRESS)
                {
                    Engine::MouseButtonPressedEvent e(l_TRButton, l_Mod);
                    a_Data.EventCallback(e);
                }
                else if (action == GLFW_RELEASE)
                {
                    Engine::MouseButtonReleasedEvent e(l_TRButton, l_Mod);
                    a_Data.EventCallback(e);
                }
            });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                (void)scancode;
                if (!a_Data.EventCallback)
                {
                    return;
                }

                const uint16_t l_Mod = ConvertGLFWMods(mods);
                const auto l_TRKey = Engine::Input::FromGLFWKey(key);

                if (action == GLFW_PRESS)
                {
                    Engine::KeyPressedEvent e(l_TRKey, l_Mod, 0);
                    a_Data.EventCallback(e);
                }
                else if (action == GLFW_RELEASE)
                {
                    Engine::KeyReleasedEvent e(l_TRKey, l_Mod);
                    a_Data.EventCallback(e);
                }
                else if (action == GLFW_REPEAT)
                {
                    Engine::KeyPressedEvent e(l_TRKey, l_Mod, 1);
                    a_Data.EventCallback(e);
                }
            });

        glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                if (a_Data.EventCallback)
                {
                    Engine::KeyTypedEvent e((uint32_t)codepoint);
                    a_Data.EventCallback(e);
                }
            });

        glfwSetWindowFocusCallback(m_Window, [](GLFWwindow* window, int focused)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                if (!a_Data.EventCallback)
                {
                    return;
                }

                if (focused)
                {
                    Engine::WindowFocusEvent e;
                    a_Data.EventCallback(e);
                }
                else
                {
                    Engine::WindowLostFocusEvent e;
                    a_Data.EventCallback(e);
                }
            });

        glfwSetDropCallback(m_Window, [](GLFWwindow* window, int count, const char** paths)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                if (!a_Data.EventCallback)
                {
                    return;
                }

                std::vector<std::string> files;
                files.reserve((size_t)count);
                for (int i = 0; i < count; i++)
                {
                    files.emplace_back(paths[i]);
                }

                Engine::WindowDropEvent e(std::move(files));
                a_Data.EventCallback(e);
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
}