#include "Engine/Renderer/RenderCommand.h"

#include "Engine/Renderer/Renderer.h"

namespace Engine
{
    namespace Render
    {
        Renderer* RenderCommand::s_Renderer = nullptr;

        void RenderCommand::Initialize(Renderer& renderer)
        {
            s_Renderer = &renderer;
        }

        void RenderCommand::Shutdown()
        {
            s_Renderer = nullptr;
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

        void RenderCommand::SetClearColor(const glm::vec4& color)
        {
            if (s_Renderer)
            {
                s_Renderer->SetClearColor(color);
            }
        }

        void RenderCommand::Clear()
        {
            if (s_Renderer)
            {
                s_Renderer->Clear();
            }
        }

        void RenderCommand::DrawCube(const glm::vec3& size, const glm::vec3& position, const glm::vec4& tint)
        {
            if (s_Renderer)
            {
                s_Renderer->DrawCube(size, position, tint);
            }
        }

        // -------- namespace wrappers --------

        void Initialize(Renderer& renderer) { RenderCommand::Initialize(renderer); }
        void Shutdown() { RenderCommand::Shutdown(); }

        void BeginFrame() { RenderCommand::BeginFrame(); }
        void EndFrame() { RenderCommand::EndFrame(); }

        void SetClearColor(const glm::vec4& color) { RenderCommand::SetClearColor(color); }
        void Clear() { RenderCommand::Clear(); }

        void DrawCube(const glm::vec3& size, const glm::vec3& position, const glm::vec4& tint)
        {
            RenderCommand::DrawCube(size, position, tint);
        }
    }
}