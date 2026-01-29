#pragma once

#include "Engine/Renderer/RendererAPI.h"

#include <memory>

namespace Engine
{
    class Window;

    namespace Render
    {
        class Camera;
    }
}

namespace Engine
{
    namespace Render
    {
        class Renderer
        {
        public:
            static void Initialize(Window* window);
            static void Shutdown();

            static bool BeginFrame();
            static void EndFrame();

            static void OnResize(uint32_t width, uint32_t height);
            static void WaitIdle();

            // Command recording
            static void SetClearColor(const glm::vec4& color);
            static void Clear();
            static void DrawTriangle();
            static void DrawCube();

            static void SetActiveCamera(Camera* camera);

            static RendererAPI& GetRendererAPI();

        private:
            static std::unique_ptr<RendererAPI> s_RendererAPI;
            static std::vector<Command> s_CommandList;
            static glm::vec4 s_ClearColor;
            static Camera* s_ActiveCamera;
        };
    }
}