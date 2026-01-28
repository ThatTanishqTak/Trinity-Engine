#include "Engine/Renderer/Renderer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Platform/Window.h"
#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

#include <cstdlib>

namespace Engine::Render
{
    std::unique_ptr<RendererAPI> Renderer::s_RendererAPI;
    std::vector<Command> Renderer::s_CommandList;
    glm::vec4 Renderer::s_ClearColor = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);

    void Renderer::Initialize(Window* window)
    {
        if (!window)
        {
            TR_CORE_CRITICAL("Renderer::Initialize called with null Window.");

            std::abort();
        }

        // For now: Vulkan only. Humans can argue about API selection later.
        s_RendererAPI = std::make_unique<VulkanRenderer>();
        s_RendererAPI->Initialize(window);

        TR_CORE_INFO("Renderer initialized (API = Vulkan).");
    }

    void Renderer::Shutdown()
    {
        if (s_RendererAPI)
        {
            s_RendererAPI->WaitIdle();
            s_RendererAPI->Shutdown();
            s_RendererAPI.reset();
        }

        s_CommandList.clear();
        TR_CORE_INFO("Renderer shutdown complete.");
    }

    void Renderer::BeginFrame()
    {
        s_CommandList.clear();
        s_RendererAPI->BeginFrame();
    }

    void Renderer::EndFrame()
    {
        // Always clear at least once per frame.
        if (s_CommandList.empty() || s_CommandList[0].Type != CommandType::Clear)
        {
            Clear();
        }

        s_RendererAPI->Execute(s_CommandList);
        s_RendererAPI->EndFrame();
    }

    void Renderer::OnResize(uint32_t width, uint32_t height)
    {
        if (s_RendererAPI)
        {
            s_RendererAPI->OnResize(width, height);
        }
    }

    void Renderer::WaitIdle()
    {
        if (s_RendererAPI)
        {
            s_RendererAPI->WaitIdle();
        }
    }

    void Renderer::SetClearColor(const glm::vec4& color)
    {
        s_ClearColor = color;
    }

    void Renderer::Clear()
    {
        Command l_Command;
        l_Command.Type = CommandType::Clear;
        l_Command.Color = s_ClearColor;
        s_CommandList.push_back(l_Command);
    }

    void Renderer::DrawTriangle()
    {
        Command l_Command;
        l_Command.Type = CommandType::DrawTriangle;
        s_CommandList.push_back(l_Command);
    }

    RendererAPI& Renderer::GetRendererAPI()
    {
        return *s_RendererAPI;
    }
}