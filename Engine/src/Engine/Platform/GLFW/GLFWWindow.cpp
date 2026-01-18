#include "Engine/Platform/GLFW/GLFWWindow.h"
#include "Engine/Core/Utilities.h"

#include <GLFW/glfw3.h>

namespace Engine
{
    static bool s_GLFWInitialized = false;
    static uint32_t s_GLFWWindowCount = 0;

    static void GLFWErrorCallback(int error, const char* description)
    {
        TR_CORE_ERROR("GLFW Error ({0}): {1}", error, description ? description : "Unknown");
    }

    GLFWWindow::GLFWWindow(const WindowProperties& props)
    {
        Initialize(props);
    }

    GLFWWindow::~GLFWWindow()
    {
        Shutdown();
    }

    void GLFWWindow::Initialize(const WindowProperties& props)
    {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;
        m_Data.VSync = props.VSync;

        if (!s_GLFWInitialized)
        {
            glfwSetErrorCallback(GLFWErrorCallback);

            if (glfwInit() != GLFW_TRUE)
            {
                TR_CORE_ERROR("Failed to initialize GLFW.");
                return;
            }

            s_GLFWInitialized = true;
            TR_CORE_INFO("GLFW initialized.");
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

        // Resize callback (framebuffer size is what matters for Vulkan swapchain)
        glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                a_Data.Width = (uint32_t)width;
                a_Data.Height = (uint32_t)height;
                a_Data.FramebufferResized = true;

                // TODO: Once you have an event system:
                // a_Data.EventCallback(WindowResizeEvent(width, height));
            });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
            {
                auto& a_Data = *(WindowData*)glfwGetWindowUserPointer(window);
                // TODO: event system later.
                // a_Data.EventCallback(WindowCloseEvent());
                (void)a_Data;
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
            TR_CORE_INFO("Window destroyed.");

            if (s_GLFWWindowCount == 0 && s_GLFWInitialized)
            {
                glfwTerminate();
                s_GLFWInitialized = false;

                TR_CORE_INFO("GLFW terminated.");
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