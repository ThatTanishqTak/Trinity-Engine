#pragma once

#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Buffer.h"

#include <glm/mat4x4.hpp>
#include <vk_mem_alloc.h>

#include <vector>

namespace Trinity
{
    struct ShadowDrawCommand
    {
        VertexBuffer* VB = nullptr;
        IndexBuffer* IB = nullptr;
        uint32_t IndexCount = 0;
        glm::mat4 LightSpaceMVP = glm::mat4(1.0f);
    };

    class VulkanShadowPass final : public VulkanRenderPass
    {
    public:
        static constexpr uint32_t s_ShadowMapSize = 2048;

    public:
        void Initialize(const VulkanPassContext& context) override;
        void Execute(const VulkanFrameContext& frameContext) override;
        void Shutdown() override;
        void Recreate(uint32_t width, uint32_t height) override;
        const char* GetName() const override { return "ShadowPass"; }

        // Submission API — called by SceneRenderer before Execute
        void Reset();
        void SetLightSpaceMatrix(const glm::mat4& matrix);
        void Submit(VertexBuffer& vb, IndexBuffer& ib, uint32_t indexCount, const glm::mat4& lightSpaceMVP);

        bool HasShadowCaster() const { return m_HasShadowCaster; }

        // Resource accessors for dependent passes
        VkImageView GetShadowMapView() const { return m_ShadowMapView; }
        VkSampler GetShadowMapSampler() const { return m_ShadowMapSampler; }
        const glm::mat4& GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }

    private:
        void CreateShadowMap();
        void CreatePipeline();
        void DestroyShadowMap();
        void DestroyPipeline();
        void InitialShadowMapTransition();

    private:
        // Pipeline
        VkPipeline m_ShadowPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_ShadowPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_ShadowSetLayout = VK_NULL_HANDLE;

        // Shadow map
        VkImage m_ShadowMapImage = VK_NULL_HANDLE;
        VmaAllocation m_ShadowMapAllocation = VK_NULL_HANDLE;
        VkImageView m_ShadowMapView = VK_NULL_HANDLE;
        VkSampler m_ShadowMapSampler = VK_NULL_HANDLE;

        // Per-frame submit data
        glm::mat4 m_LightSpaceMatrix = glm::mat4(1.0f);
        bool m_HasShadowCaster = false;
        std::vector<ShadowDrawCommand> m_DrawCommands;
    };
}