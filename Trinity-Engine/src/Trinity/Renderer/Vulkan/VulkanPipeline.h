#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <cstdint>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanPipeline
	{
	public:
		void Initialize(const VulkanContext& context, const VulkanDevice& device, VkFormat colorFormat, VkFormat depthFormat,
			const std::vector<uint32_t>& vertexSpirV, const std::vector<uint32_t>& fragmentSpirV);
		void Shutdown();

		void Recreate(VkFormat colorFormat);

		void Bind(VkCommandBuffer commandBuffer) const;

		VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

	private:
		void CreatePipeline();
		void DestroyPipeline();

		VkShaderModule CreateShaderModule(const std::vector<uint32_t>& spirV) const;

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;

		VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;
		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;

		std::vector<uint32_t> m_VertexSpirV;
		std::vector<uint32_t> m_FragmentSpirV;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}