#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanPipeline
	{
	public:
		void Initialize(const VulkanContext& context, const VulkanDevice& device, VkFormat colorFormat, const std::string& vertexShaderSpvPath, const std::string& fragmentShaderSpvPath);
		void Shutdown();

		void Recreate(VkFormat colorFormat);

		void Bind(VkCommandBuffer commandBuffer) const;

		VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

	private:
		void CreatePipeline();
		void DestroyPipeline();

		VkShaderModule CreateShaderModule(const std::string& path) const;

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;

		VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;

		std::string m_VertexShaderPath{};
		std::string m_FragmentShaderPath{};

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}