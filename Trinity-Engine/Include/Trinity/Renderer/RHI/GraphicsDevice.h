#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>
#include <Trinity/Renderer/RHI/Buffer.h>
#include <Trinity/Renderer/RHI/Texture.h>
#include <Trinity/Renderer/RHI/Shader.h>
#include <Trinity/Renderer/RHI/Pipeline.h>
#include <Trinity/Renderer/RHI/CommandList.h>
#include <Trinity/Renderer/RHI/Swapchain.h>

namespace Trinity
{
    struct DeviceCapabilities
    {
        std::string DeviceName;
        
        uint64_t DedicatedVideoMemory = 0;
        uint32_t MaxTexture2DSize = 0;
        uint32_t MaxPushConstantSize = 0;
        uint32_t MaxColorAttachments = 1;

        bool SupportsAnisotropy = false;
        bool SupportsRayTracing = false;
    };

    class GraphicsDevice
    {
    public:
        virtual ~GraphicsDevice() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual GraphicsBackend GetBackend() const = 0;
        virtual const DeviceCapabilities& GetCapabilities() const = 0;

        virtual BufferHandle CreateBuffer(const BufferDescription& description) = 0;
        virtual TextureHandle CreateTexture(const TextureDescription& description) = 0;
        virtual SamplerHandle CreateSampler(const SamplerDescription& description) = 0;
        virtual ShaderHandle CreateShader(const ShaderDescription& description) = 0;
        virtual PipelineHandle CreatePipeline(const PipelineDescription& description) = 0;

        virtual void DestroyBuffer(BufferHandle handle) = 0;
        virtual void DestroyTexture(TextureHandle handle) = 0;
        virtual void DestroySampler(SamplerHandle handle) = 0;
        virtual void DestroyShader(ShaderHandle handle) = 0;
        virtual void DestroyPipeline(PipelineHandle handle) = 0;

        virtual void UpdateBuffer(BufferHandle handle, const void* data, uint64_t size, uint64_t offset = 0) = 0;

        virtual std::unique_ptr<Swapchain> CreateSwapchain(const SwapchainDescription& description) = 0;
        virtual std::unique_ptr<CommandList> CreateCommandList() = 0;

        virtual void Submit(CommandList& commandList) = 0;
        virtual void WaitIdle() = 0;
    };
}