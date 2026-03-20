#pragma once

#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vk_mem_alloc.h>

#include <vector>

namespace Trinity
{
    struct LightingSubmitData
    {
        LightingUniformData UniformData{};
        glm::mat4 InvViewProjection = glm::mat4(1.0f);
        glm::vec3 CameraPosition = glm::vec3(0.0f);
        float CameraNear = 0.01f;
        float CameraFar = 1000.0f;
    };

    class VulkanLightingPass final : public VulkanRenderPass
    {
    public:
        void Initialize(const VulkanPassContext& context) override;
        void Execute(const VulkanFrameContext& frameContext) override;
        void Shutdown() override;
        void Recreate(uint32_t width, uint32_t height) override;
        const char* GetName() const override { return "LightingPass"; }

        // Cross-pass resource injection — called by VulkanRenderer after all passes are initialized
        void SetGBufferResources(VkImageView albedo, VkImageView normal, VkImageView material, VkImageView depth);
        void SetShadowResources(VkImageView shadowMapView, VkSampler shadowMapSampler);

        // Submission API
        void Submit(const LightingSubmitData& data);

        // Resource accessors for dependent passes
        VkImageView GetSceneViewportImageView() const { return m_SceneViewportImageView; }
        VkSampler GetSceneViewportSampler() const { return m_SceneViewportSampler; }

    private:
        void CreateSceneViewportImage(uint32_t width, uint32_t height);
        void DestroySceneViewportImage();
        void CreatePipeline();
        void DestroyPipeline();

    private:
        // Scene viewport output (written by lighting, read by post-process)
        VkImage m_SceneViewportImage = VK_NULL_HANDLE;
        VmaAllocation m_SceneViewportImageAllocation = VK_NULL_HANDLE;
        VkImageView m_SceneViewportImageView = VK_NULL_HANDLE;
        VkSampler m_SceneViewportSampler = VK_NULL_HANDLE;

        // Shared GBuffer sampler (no compare, used for albedo/normal/material/depth reads)
        VkSampler m_GBufferSampler = VK_NULL_HANDLE;

        // Cross-pass resource handles (set by VulkanRenderer after initialization)
        VkImageView m_AlbedoView = VK_NULL_HANDLE;
        VkImageView m_NormalView = VK_NULL_HANDLE;
        VkImageView m_MaterialView = VK_NULL_HANDLE;
        VkImageView m_DepthView = VK_NULL_HANDLE;
        VkImageView m_ShadowMapView = VK_NULL_HANDLE;
        VkSampler m_ShadowMapSampler = VK_NULL_HANDLE;

        // Pipeline
        VkPipeline m_LightingPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_LightingPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_LightingGBufferSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_LightingUBOSetLayout = VK_NULL_HANDLE;

        // Per-frame UBOs and descriptor pools
        std::vector<VulkanUniformBuffer> m_LightingUBOs;
        std::vector<VkDescriptorPool> m_LightingDescriptorPools;

        // Per-frame submit data
        LightingSubmitData m_SubmitData{};
        bool m_HasSubmitData = false;
    };
}