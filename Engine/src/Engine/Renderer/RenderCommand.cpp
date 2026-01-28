#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/Renderer.h"

namespace Engine::Render
{
    void RenderCommand::SetClearColor(const glm::vec4& color)
    {
        Renderer::SetClearColor(color);
    }

    void RenderCommand::Clear()
    {
        Renderer::Clear();
    }

    void RenderCommand::DrawTriangle()
    {
        Renderer::DrawTriangle();
    }
}