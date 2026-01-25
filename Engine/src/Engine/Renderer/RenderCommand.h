#pragma once

#include <glm/glm.hpp>

namespace Engine
{
    class Renderer;

    namespace Render
    {
        class RenderCommand
        {
        public:
            // Call once after the Renderer backend is created.
            static void Initialize(Renderer& renderer);
            static void Shutdown();

            static void BeginFrame();
            static void EndFrame();

            static void SetClearColor(const glm::vec4& color);
            static void Clear();

            static void DrawCube(const glm::vec3& size, const glm::vec3& position, const glm::vec4& tint);

        private:
            static Renderer* s_Renderer;
        };

        void Initialize(Renderer& renderer);
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        void SetClearColor(const glm::vec4& color);
        void Clear();

        void DrawCube(const glm::vec3& size, const glm::vec3& position, const glm::vec4& tint);
    }
}