#include "Engine/Renderer/Renderer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Platform/Window.h"
#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

namespace Engine
{
    Renderer::Renderer()
    {

    }

    Renderer::~Renderer() = default;

    void Renderer::Initialize(Window& window)
    {
        TR_CORE_INFO("Renderer: Creating Vulkan backend...");

        m_Vulkan = std::make_unique<VulkanRenderer>();
        m_Vulkan->Initialize(window);
    }

    void Renderer::Shutdown()
    {

    }

    void Renderer::BeginFrame()
    {
        if (m_Vulkan) m_Vulkan->BeginFrame();
    }

    void Renderer::EndFrame()
    {
        if (m_Vulkan) m_Vulkan->EndFrame();
    }

    void Renderer::OnResize(uint32_t width, uint32_t height)
    {
        if (m_Vulkan) m_Vulkan->OnResize(width, height);
    }
}