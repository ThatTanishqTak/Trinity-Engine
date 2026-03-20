#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Renderer/RendererFactory.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderGraph.h"
#include "Trinity/Utilities/Log.h"

#include <memory>

namespace Trinity
{
    static std::unique_ptr<Renderer> s_Renderer = nullptr;

    void RenderCommand::Initialize(Window& window, RendererAPI api)
    {
        if (s_Renderer != nullptr)
        {
            TR_CORE_WARN("RenderCommand::Initialize called while renderer already exists. Reinitializing.");

            s_Renderer->Shutdown();
            s_Renderer.reset();
        }

        s_Renderer = RendererFactory::Create(api);
        if (!s_Renderer)
        {
            TR_CORE_CRITICAL("RenderCommand::Initialize: RendererFactory returned nullptr");

            std::abort();
        }

        TR_CORE_TRACE("Selected API: {}", ApiToString(api));

        s_Renderer->SetWindow(window);
        s_Renderer->Initialize();
    }

    void RenderCommand::Shutdown()
    {
        if (!s_Renderer)
        {
            return;
        }

        s_Renderer->Shutdown();
        s_Renderer.reset();
    }

    void RenderCommand::Resize(uint32_t width, uint32_t height)
    {
        if (s_Renderer)
        {
            s_Renderer->Resize(width, height);
        }
    }

    void RenderCommand::BeginFrame()
    {
        if (s_Renderer)
        {
            s_Renderer->BeginFrame();
        }
    }

    void RenderCommand::EndFrame()
    {
        if (s_Renderer)
        {
            s_Renderer->EndFrame();
        }
    }

    void RenderCommand::RenderImGui(ImGuiLayer& imGuiLayer)
    {
        if (s_Renderer)
        {
            s_Renderer->RenderImGui(imGuiLayer);
        }
    }

    void RenderCommand::SetSceneViewportSize(uint32_t width, uint32_t height)
    {
        if (s_Renderer)
        {
            s_Renderer->SetSceneViewportSize(width, height);
        }
    }

    void* RenderCommand::GetSceneViewportHandle()
    {
        if (!s_Renderer)
        {
            return nullptr;
        }

        return s_Renderer->GetSceneViewportHandle();
    }

    Renderer& RenderCommand::GetRenderer()
    {
        if (!s_Renderer)
        {
            TR_CORE_CRITICAL("RenderCommand::GetRenderer called before Initialize");

            std::abort();
        }

        return *s_Renderer;
    }

    VulkanRenderGraph& RenderCommand::GetRenderGraph()
    {
        if (!s_Renderer)
        {
            TR_CORE_CRITICAL("RenderCommand::GetRenderGraph called before Initialize");

            std::abort();
        }

        if (s_Renderer->GetAPI() != RendererAPI::VULKAN)
        {
            TR_CORE_CRITICAL("RenderCommand::GetRenderGraph called on a non-Vulkan renderer");

            std::abort();
        }

        return static_cast<VulkanRenderer&>(*s_Renderer).GetRenderGraph();
    }

    std::string RenderCommand::ApiToString(RendererAPI api)
    {
        switch (api)
        {
            case RendererAPI::VULKAN:
                return "Vulkan";
            case RendererAPI::MOLTENVK:
                return "MoltenVK";
            case RendererAPI::DIRECTX:
                return "DirectX 12";
            default:
                return "None";
        }
    }
}