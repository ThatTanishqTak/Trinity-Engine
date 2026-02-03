#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Trinity
{
	class VulkanDevice;
	class VulkanRenderPass;
	class VulkanDescriptors;

	class VulkanPipeline
	{
	public:
		struct GraphicsDescription
		{
			VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
			VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
			VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

			bool EnableDepthTest = false;
			bool EnableDepthWrite = false;
			VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

			bool EnableBlending = false;

			std::vector<VkDynamicState> DynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
		};

	public:
		VulkanPipeline() = default;
		~VulkanPipeline() = default;

		VulkanPipeline(const VulkanPipeline&) = delete;
		VulkanPipeline& operator=(const VulkanPipeline&) = delete;
		VulkanPipeline(VulkanPipeline&&) = delete;
		VulkanPipeline& operator=(VulkanPipeline&&) = delete;

		void Initialize(const VulkanDevice& device, const VulkanRenderPass& renderPass, const VulkanDescriptors& descriptors, const std::string& vertexSPVPath,
			const std::string& fragmentSpvPath, VkAllocationCallbacks* allocator = nullptr, bool enablePipelineCache = true);

		void Shutdown();

		// Rebuild pipeline if render pass handle changed (swapchain format changes -> new render pass).
		void Recreate(const VulkanRenderPass& renderPass);

		void SetGraphicsDescription(const GraphicsDescription& description) { m_GraphicsDescription = description; }

		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineCache GetPipelineCache() const { return m_PipelineCache; }

	private:
		void CreatePipelineCache();
		void DestroyPipelineCache();

		void CreatePipelineLayout(const VulkanDescriptors& descriptors);
		void DestroyPipelineLayout();

		void CreateGraphicsPipeline(VkRenderPass renderPass);
		void DestroyGraphicsPipeline();

		VkShaderModule GetOrCreateShaderModule(const std::string& SPVPath);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		std::string m_VertexSPVPath;
		std::string m_FragmentSPVPath;

		GraphicsDescription m_GraphicsDescription{};

		// SPIR-V path -> VkShaderModule
		std::unordered_map<std::string, VkShaderModule> m_ShaderModules;
	};
}