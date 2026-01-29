#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Engine
{
    namespace Render
    {
        class RenderCommand
        {
        public:
            static void SetClearColor(const glm::vec4& color);
            static void Clear();

            // Debug primitive for sanity (and for keeping you from spiraling).
            static void DrawTriangle();
            static void DrawCube();
        };
    }
}