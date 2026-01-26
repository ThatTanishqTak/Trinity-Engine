#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <span>
#include <string>
#include <vector>

namespace Engine
{
    class VulkanDevice;

    class VulkanPipeline
    {
    public:
        struct GraphicsPipelineDescription
        {
            std::string VertexShaderPath;
            std::string FragmentShaderPath;

            VkExtent2D Extent{};

            // Optional knobs for later expansion.
            bool EnableDepth = false;
            VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
            VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            uint32_t PushConstantSize = 0;
            VkShaderStageFlags PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;

            // Where to store/load pipeline cache on disk.
            std::string PipelineCachePath = "Cache/pipeline_cache.bin";
        };

    public:
        VulkanPipeline() = default;
        ~VulkanPipeline() = default;

        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;
        VulkanPipeline(VulkanPipeline&&) = delete;
        VulkanPipeline& operator=(VulkanPipeline&&) = delete;

        void Initialize(VulkanDevice& device, VkRenderPass renderPass, const GraphicsPipelineDescription& description, std::span<const VkDescriptorSetLayout> descriptorSetLayouts);
        void Shutdown(VulkanDevice& device);
        void Shutdown(VulkanDevice& device, const std::function<void(std::function<void()>&&)>& submitResourceFree);

        void Recreate(VulkanDevice& device, VkRenderPass renderPass, const GraphicsPipelineDescription& description, std::span<const VkDescriptorSetLayout> descriptorSetLayouts);

        bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE && m_PipelineLayout != VK_NULL_HANDLE; }

        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
        VkPipelineCache GetPipelineCache() const { return m_PipelineCache; }

    private:
        VkShaderModule CreateShaderModule(VulkanDevice& device, const std::string& path);

        void CreatePipelineCache(VulkanDevice& device, const std::string& cachePath);
        void SavePipelineCache(VulkanDevice& device, const std::string& cachePath);

    private:
        VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        VkExtent2D m_Extent{};

        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

        std::string m_VertexShaderPath;
        std::string m_FragmentShaderPath;
        std::string m_PipelineCachePath;

        bool m_Initialized = false;
    };
}