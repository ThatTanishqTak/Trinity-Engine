#pragma once

#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanGeometryBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanTexture.h"
#include "Trinity/Renderer/Buffer.h"
#include "Trinity/Renderer/Texture.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vk_mem_alloc.h>

#include <vector>

namespace Trinity
{
    struct GeometryDrawCommand
    {
        VertexBuffer* VB = nullptr;
        IndexBuffer* IB = nullptr;
        uint32_t IndexCount = 0;
        glm::mat4 Model = glm::mat4(1.0f);
        glm::mat4 ViewProjection = glm::mat4(1.0f);
        glm::vec4 Color = glm::vec4(1.0f);
        VulkanTexture2D* AlbedoTexture = nullptr;
    };

    class VulkanGeometryPass final : public VulkanRenderPass
    {
    public:
        static constexpr uint32_t s_MaxTextureDescriptorsPerFrame = 1024;

    public:
        void Initialize(const VulkanPassContext& context) override;
        void Execute(const VulkanFrameContext& frameContext) override;
        void Shutdown() override;
        void Recreate(uint32_t width, uint32_t height) override;
        const char* GetName() const override { return "GeometryPass"; }

        // Submission API
        void Reset();
        void Submit(const GeometryDrawCommand& command);

        // Resource accessors for dependent passes
        VkImageView GetAlbedoView() const { return m_GBuffer.GetAlbedoView(); }
        VkImageView GetNormalView() const { return m_GBuffer.GetNormalView(); }
        VkImageView GetMaterialView() const { return m_GBuffer.GetMaterialView(); }
        VkImageView GetDepthImageView() const { return m_DepthImageView; }

    private:
        void CreateDepthImage(uint32_t width, uint32_t height);
        void DestroyDepthImage();
        void CreatePipeline();
        void DestroyPipeline();

    private:
        // GBuffer
        VulkanGeometryBuffer m_GBuffer{};

        // Depth image (shared with LightingPass for world-position reconstruction)
        VkImage m_DepthImage = VK_NULL_HANDLE;
        VmaAllocation m_DepthImageAllocation = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;

        // Pipeline
        VkPipeline m_GBufferPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_GBufferPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_GBufferTextureSetLayout = VK_NULL_HANDLE;

        // Per-frame texture descriptor allocator
        VulkanDescriptorAllocator m_DescriptorAllocator{};

        // 1×1 white fallback texture
        VulkanTexture2D m_WhiteTexture{};

        // Per-frame submit data
        std::vector<GeometryDrawCommand> m_DrawCommands;
    };
}