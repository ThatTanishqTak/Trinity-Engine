#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Framebuffer.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Sampler.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>

namespace Trinity
{
    class Window;

    enum class RendererBackend : uint8_t
    {
        None = 0,
        Vulkan,
        DirectX12,
        Metal
    };

    struct RendererAPISpecification
    {
        uint32_t MaxFramesInFlight = 2;
        bool EnableValidation = false;
    };

    class RendererAPI
    {
    public:
        virtual ~RendererAPI() = default;

        virtual void Initialize(Window& window, const RendererAPISpecification& specification) = 0;
        virtual void Shutdown() = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void Present() = 0;
        virtual void WaitIdle() = 0;

        virtual std::shared_ptr<Buffer> CreateBuffer(const BufferSpecification& specification) = 0;
        virtual std::shared_ptr<Texture> CreateTexture(const TextureSpecification& specification) = 0;
        virtual std::shared_ptr<Texture> CreateTextureFromData(const void* data, uint32_t width, uint32_t height) = 0;
        virtual std::shared_ptr<Texture> CreateTextureFromMemory(const uint8_t* data, size_t size) = 0;
        virtual std::shared_ptr<Texture> LoadTextureFromFile(const std::string& path) = 0;
        virtual std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferSpecification& specification) = 0;
        virtual std::shared_ptr<Shader> CreateShader(const ShaderSpecification& specification) = 0;
        virtual std::shared_ptr<Pipeline> CreatePipeline(const PipelineSpecification& specification) = 0;
        virtual std::shared_ptr<Sampler> CreateSampler(const SamplerSpecification& specification) = 0;

        virtual void OnWindowResize(uint32_t width, uint32_t height) = 0;
        virtual uint32_t GetSwapchainWidth() const = 0;
        virtual uint32_t GetSwapchainHeight() const = 0;
        virtual uint32_t GetCurrentFrameIndex() const = 0;
        virtual uint32_t GetMaxFramesInFlight() const = 0;

        RendererBackend GetBackend() const { return m_Backend; }

        static std::unique_ptr<RendererAPI> Create(RendererBackend backend);

    protected:
        RendererBackend m_Backend = RendererBackend::None;
    };
}