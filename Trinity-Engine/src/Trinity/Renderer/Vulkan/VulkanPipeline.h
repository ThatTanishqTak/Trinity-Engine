#pragma once

#include "Trinity/Geometry/Vertex.h"
#include "Trinity/Renderer/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Trinity
{
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

			std::vector<VkVertexInputBindingDescription> VertexBindings =
			{
				{
					.binding = 0,
					.stride = sizeof(Geometry::Vertex),
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
				}
			};

			std::vector<VkVertexInputAttributeDescription> VertexAttributes =
			{
				{
					.location = 0,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Geometry::Vertex, Position)
				},
				{
					.location = 1,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Geometry::Vertex, Normal)
				},
				{
					.location = 2,
					.binding = 0,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Geometry::Vertex, UV)
				},
				{
					.location = 3,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.offset = offsetof(Geometry::Vertex, Tangent)
				}
			};

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

		void Initialize(const VulkanContext& context, const VulkanRenderPass& renderPass, const VulkanDescriptors& descriptors,
			const std::string& vertexSPVPath, const std::string& fragmentSpvPath, bool enablePipelineCache = true);

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
		std::string BuildPipelineCachePath(const VulkanContext& context, const std::string& vertexPath, const std::string& fragmentPath);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		std::string m_VertexSPVPath;
		std::string m_FragmentSPVPath;
		std::string m_PipelineCachePath;
		bool m_EnablePipelineCache = true;

		GraphicsDescription m_GraphicsDescription{};

		// SPIR-V path -> VkShaderModule
		std::unordered_map<std::string, VkShaderModule> m_ShaderModules;
	};
}