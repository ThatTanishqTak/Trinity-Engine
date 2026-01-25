#pragma once

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>

namespace Engine
{
    class Window;

    class Renderer
    {
    public:
        virtual ~Renderer() = default;

        virtual void Initialize(Window& window) = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        // High-level draw interface (front-end). Backends decide how to implement.
        virtual void SetClearColor(const glm::vec4& a_Color) = 0;
        virtual void Clear() = 0;
        virtual void DrawCube(const glm::vec3& a_Size, const glm::vec3& a_Position, const glm::vec4& a_Tint) = 0;

        virtual void OnResize(uint32_t width, uint32_t height) = 0;

        static std::unique_ptr<Renderer> Create();
    };
}