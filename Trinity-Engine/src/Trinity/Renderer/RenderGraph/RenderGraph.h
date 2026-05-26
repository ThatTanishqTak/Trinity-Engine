#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphPass.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphResource.h"
#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Trinity
{
    using ResourceLifetime = RenderGraphResourceLifetime;

    class RenderGraph
    {
    public:
        RenderGraph() = default;
        virtual ~RenderGraph() = default;

        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;

        RenderGraphResourceHandle DeclareTexture(const RenderGraphTextureDescription& description);
        RenderGraphResourceHandle DeclareBuffer(const RenderGraphBufferDescription& description);

        RenderGraphResourceHandle ImportTexture(const std::string& name, const std::shared_ptr<Texture>& texture, RenderGraphAccess initialAccess = RenderGraphAccess::ShaderSampledRead);
        RenderGraphResourceHandle ImportBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer, RenderGraphAccess initialAccess = RenderGraphAccess::StorageBufferRead);

        void SetImportedTexture(RenderGraphResourceHandle handle, const std::shared_ptr<Texture>& texture);
        void MarkOutput(RenderGraphResourceHandle handle);

        RenderGraphPass& AddPass(const std::string& name);

        void Reset();

        bool Validate();
        void Compile();
        void Execute();

        void OnResize(uint32_t width, uint32_t height);

        std::shared_ptr<Texture> GetTexture(RenderGraphResourceHandle handle) const;
        std::shared_ptr<Buffer> GetBuffer(RenderGraphResourceHandle handle) const;

        RenderGraphResourceHandle GetResourceHandle(const std::string& name) const;
        RenderGraphPass* GetPass(const std::string& name);
        const RenderGraphPass* GetPass(const std::string& name) const;

        std::string GetResourceName(RenderGraphResourceHandle handle) const;
        std::string DumpText() const;
        std::string DumpDot() const;

        bool IsCompiled() const { return m_Compiled; }

        const std::vector<RenderGraphPass>& GetPasses() const { return m_Passes; }
        const std::vector<uint32_t>& GetExecutionOrder() const { return m_ExecutionOrder; }
        const std::vector<RenderGraphTextureDescription>& GetTextureDescriptions() const { return m_TextureDescriptions; }
        const std::vector<RenderGraphBufferDescription>& GetBufferDescriptions() const { return m_BufferDescriptions; }
        const std::vector<RenderGraphTextureDescription>& GetResourceDescription() const { return m_TextureDescriptions; }
        const std::vector<RenderGraphResourceLifetime>& GetTextureLifetimes() const { return m_TextureLifetimes; }
        const std::vector<RenderGraphResourceLifetime>& GetBufferLifetimes() const { return m_BufferLifetimes; }
        const std::vector<RenderGraphResourceLifetime>& GetLifetimes() const { return m_TextureLifetimes; }
        const std::vector<std::string>& GetValidationErrors() const { return m_ValidationErrors; }

    protected:
        virtual std::vector<std::shared_ptr<Texture>> AllocateTextureBatch(const std::vector<RenderGraphTextureDescription>& descriptions, const std::vector<RenderGraphResourceLifetime>& lifetimes) = 0;
        virtual std::shared_ptr<Buffer> AllocateBuffer(const RenderGraphBufferDescription& description) = 0;

        virtual std::vector<std::shared_ptr<Texture>> AllocateResourceBatch(const std::vector<RenderGraphTextureDescription>& descriptions, const std::vector<ResourceLifetime>& lifetimes) { return AllocateTextureBatch(descriptions, lifetimes); }

        virtual void OnReset()
        {

        }

        virtual void OnCompile()
        {

        }

        virtual void OnExecutePassBegin(uint32_t passIndex, RenderGraphContext& context)
        {
            (void)passIndex;
            (void)context;
        }

        virtual void OnExecutePassEnd(uint32_t passIndex, RenderGraphContext& context)
        {
            (void)passIndex;
            (void)context;
        }

    protected:
        uint32_t m_SwapchainWidth = 0;
        uint32_t m_SwapchainHeight = 0;

        bool m_Compiled = false;

        std::vector<RenderGraphTextureDescription> m_TextureDescriptions;
        std::vector<RenderGraphBufferDescription> m_BufferDescriptions;

        std::vector<std::shared_ptr<Texture>> m_Textures;
        std::vector<std::shared_ptr<Buffer>> m_Buffers;

        std::vector<RenderGraphAccess> m_TextureInitialAccesses;
        std::vector<RenderGraphAccess> m_BufferInitialAccesses;

        std::vector<RenderGraphPass> m_Passes;
        std::vector<uint32_t> m_ExecutionOrder;

        std::vector<RenderGraphResourceLifetime> m_TextureLifetimes;
        std::vector<RenderGraphResourceLifetime> m_BufferLifetimes;

        std::vector<std::string> m_ValidationErrors;

        std::unordered_map<std::string, RenderGraphResourceHandle> m_NamedResources;
        std::unordered_map<std::string, uint32_t> m_NamedPasses;

    private:
        bool IsValidHandle(RenderGraphResourceHandle handle) const;

        void BuildExecutionOrder();
        void CullPasses();
        void ComputeLifetimes();
        void AllocateResources();

        void TouchResourceLifetime(RenderGraphResourceHandle handle, uint32_t orderIndex, bool writer);
        bool IsResourceMarkedOutput(RenderGraphResourceHandle handle) const;

        static const char* AccessToString(RenderGraphAccess access);
        static const char* PassTypeToString(RenderGraphPassType type);
    };
}