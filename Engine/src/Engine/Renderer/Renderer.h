#pragma once

#include <cstdint>
#include <memory>

namespace Engine
{
    class Window;
    class VulkanRenderer;

    class Renderer
    {
    public:
        explicit Renderer();
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = default;
        Renderer& operator=(Renderer&&) noexcept = default;

        void Initialize(Window& window);
        void Shutdown();

        void BeginFrame();
        void EndFrame();
        void OnResize(uint32_t width, uint32_t height);

    private:
        std::unique_ptr<VulkanRenderer> m_Vulkan;
    };
}