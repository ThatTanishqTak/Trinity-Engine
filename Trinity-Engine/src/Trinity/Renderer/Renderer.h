#pragma once

#include "Trinity/Renderer/RendererAPI.h"

namespace Trinity
{
    class Window;

    struct RendererSpecification
    {
        RendererBackend Backend = RendererBackend::Vulkan;
        uint32_t MaxFramesInFlight = 2;

        bool EnableValidation = false;
    };

    class Renderer
    {
    public:
        static void Initialize(Window& window, const RendererSpecification& specification);
        static void Shutdown();

        static bool BeginFrame();
        static void EndFrame();
        static void Present();
        static void WaitIdle();

        static void OnWindowResize(uint32_t width, uint32_t height);

        static std::shared_ptr<Texture> CreateTextureFromData(const void* data, uint32_t width, uint32_t height);
        static std::shared_ptr<Texture> CreateTextureFromMemory(const uint8_t* data, size_t size);
        static std::shared_ptr<Texture> LoadTextureFromFile(const std::string& path);

        static RendererAPI& GetAPI() { return *s_API; }
        static RendererBackend GetBackend();
        static uint32_t GetCurrentFrameIndex();
        static uint32_t GetMaxFramesInFlight();

    private:
        static std::unique_ptr<RendererAPI> s_API;
    };
}