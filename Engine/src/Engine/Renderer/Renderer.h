#pragma once

#include <cstdint>
#include <memory>

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

        virtual void OnResize(uint32_t width, uint32_t height) = 0;

        static std::unique_ptr<Renderer> Create();
    };
}