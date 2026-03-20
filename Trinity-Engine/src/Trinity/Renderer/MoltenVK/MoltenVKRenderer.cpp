#include "Trinity/Renderer/MoltenVK/MoltenVKRenderer.h"

#include "Trinity/Utilities/Log.h"

// WILL BE WORKED ON IN THE FUTURE

namespace Trinity
{
    MoltenVKRenderer::MoltenVKRenderer() : Renderer(RendererAPI::MOLTENVK)
    {

    }

    MoltenVKRenderer::~MoltenVKRenderer() = default;

    void MoltenVKRenderer::SetWindow(Window& window)
    {
        (void)window;
    }

    void MoltenVKRenderer::Initialize()
    {
        TR_CORE_TRACE("Initializing MoltenVK (stub)");
        TR_CORE_TRACE("MoltenVK Initialized (stub)");
    }

    void MoltenVKRenderer::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down MoltenVK (stub)");
        TR_CORE_TRACE("MoltenVK Shutdown Complete (stub)");
    }

    void MoltenVKRenderer::Resize(uint32_t width, uint32_t height)
    {
        (void)width;
        (void)height;
    }

    void MoltenVKRenderer::BeginFrame()
    {

    }

    void MoltenVKRenderer::EndFrame()
    {

    }

    void MoltenVKRenderer::RenderImGui(ImGuiLayer& imGuiLayer)
    {
        (void)imGuiLayer;
    }
    
    void MoltenVKRenderer::SetSceneViewportSize(uint32_t width, uint32_t height)
    {
        (void)width;
        (void)height;
    }

    void* MoltenVKRenderer::GetSceneViewportHandle() const
    {
        return nullptr;
    }
}