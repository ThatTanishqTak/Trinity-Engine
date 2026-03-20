#pragma once

#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.h"

#include <vk_mem_alloc.h>

namespace Trinity
{
    class VulkanPostProcessPass final : public VulkanRenderPass
    {
    public:
        void Initialize(const VulkanPassContext& context) override;
        void Execute(const VulkanFrameContext& frameContext) override;
        void Shutdown() override;
        void Recreate(uint32_t width, uint32_t height) override;
        const char* GetName() const override { return "PostProcessPass"; }

        // Cross-pass resource injection — called by VulkanRenderer after all passes are initialized
        void SetSceneColorResources(VkImageView sceneColorView, VkSampler sceneColorSampler);

        // Final output handle for ImGui::Image
        void* GetOutputHandle() const { return m_PostProcessHandle; }

    private:
        void CreateOutputImage(uint32_t width, uint32_t height);
        void DestroyOutputImage();
        void CreatePipeline();
        void DestroyPipeline();
        void RebuildImGuiHandle();

    private:
        // Post-process output image (shown in editor viewport)
        VkImage m_PostProcessImage = VK_NULL_HANDLE;
        VmaAllocation m_PostProcessImageAllocation = VK_NULL_HANDLE;
        VkImageView m_PostProcessImageView = VK_NULL_HANDLE;
        VkSampler m_PostProcessSampler = VK_NULL_HANDLE;
        void* m_PostProcessHandle = nullptr;

        // Pipeline
        VkPipeline m_PostProcessPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PostProcessPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_PostProcessSetLayout = VK_NULL_HANDLE;

        // Per-frame descriptor allocator (1 set per frame for the scene color input)
        VulkanDescriptorAllocator m_DescriptorAllocator{};

        // Cross-pass resources (set by VulkanRenderer after initialization)
        VkImageView m_SceneColorView = VK_NULL_HANDLE;
        VkSampler m_SceneColorSampler = VK_NULL_HANDLE;
    };
}