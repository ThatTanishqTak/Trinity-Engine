#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"

#include "Trinity/Geometry/Geometry.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"

#include "Trinity/Utilities/FileManagement.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>
#include <vector>
#include <cstddef>

namespace Trinity
{
	void VulkanPipeline::Initialize(const VulkanContext& context, const VulkanDevice& device, VkFormat colorFormat,
		const std::string& vertexShaderSpvPath, const std::string& fragmentShaderSpvPath)
	{
		TR_CORE_TRACE("Initializing Vulkan Pipeline");

		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_WARN("VulkanPipeline::Initialize called while already initialized. Reinitializing.");

			Shutdown();
		}

		m_Device = device.GetDevice();
		m_Allocator = context.GetAllocator();
		m_ColorFormat = colorFormat;
		m_VertexShaderPath = vertexShaderSpvPath;
		m_FragmentShaderPath = fragmentShaderSpvPath;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Initialize called with invalid VkDevice");

			std::abort();
		}

		if (m_ColorFormat == VK_FORMAT_UNDEFINED)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Initialize called with VK_FORMAT_UNDEFINED");

			std::abort();
		}

		CreatePipeline();

		TR_CORE_TRACE("Vulkan Pipeline Initialized (ColorFormat: {})", static_cast<int>(m_ColorFormat));
	}

	void VulkanPipeline::Shutdown()
	{
		DestroyPipeline();

		m_VertexShaderPath.clear();
		m_FragmentShaderPath.clear();
		m_ColorFormat = VK_FORMAT_UNDEFINED;
		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
	}

	void VulkanPipeline::Recreate(VkFormat colorFormat)
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		if (colorFormat == VK_FORMAT_UNDEFINED)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Recreate called with VK_FORMAT_UNDEFINED");

			std::abort();
		}

		if (m_ColorFormat == colorFormat && m_Pipeline != VK_NULL_HANDLE)
		{
			return;
		}

		m_ColorFormat = colorFormat;

		DestroyPipeline();
		CreatePipeline();

		TR_CORE_TRACE("VulkanPipeline recreated (ColorFormat: {})", static_cast<int>(m_ColorFormat));
	}

	void VulkanPipeline::Bind(VkCommandBuffer commandBuffer) const
	{
		if (m_Pipeline == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Bind called with null pipeline");

			std::abort();
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}

	void VulkanPipeline::CreatePipeline()
	{
		if (m_Pipeline != VK_NULL_HANDLE || m_PipelineLayout != VK_NULL_HANDLE)
		{
			DestroyPipeline();
		}

		const VkShaderModule l_VertexShaderModule = CreateShaderModule(m_VertexShaderPath);
		const VkShaderModule l_FragmentShaderModule = CreateShaderModule(m_FragmentShaderPath);

		VkPipelineShaderStageCreateInfo l_VertexStage{};
		l_VertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_VertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_VertexStage.module = l_VertexShaderModule;
		l_VertexStage.pName = "main";

		VkPipelineShaderStageCreateInfo l_FragmentStage{};
		l_FragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_FragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_FragmentStage.module = l_FragmentShaderModule;
		l_FragmentStage.pName = "main";

		const VkPipelineShaderStageCreateInfo l_ShaderStageCreateInfo[] = { l_VertexStage, l_FragmentStage };

		// Vertex input
		VkVertexInputBindingDescription l_BindingDescription{};
		l_BindingDescription.binding = 0;
		l_BindingDescription.stride = sizeof(Geometry::Vertex);
		l_BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription l_AttributeDescription[3]{};
		l_AttributeDescription[0].location = 0; l_AttributeDescription[0].binding = 0;
		l_AttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		l_AttributeDescription[0].offset = offsetof(Geometry::Vertex, Position);
		l_AttributeDescription[1].location = 1; l_AttributeDescription[1].binding = 0;
		l_AttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		l_AttributeDescription[1].offset = offsetof(Geometry::Vertex, Normal);
		l_AttributeDescription[2].location = 2; l_AttributeDescription[2].binding = 0;
		l_AttributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
		l_AttributeDescription[2].offset = offsetof(Geometry::Vertex, UV);

		VkPipelineVertexInputStateCreateInfo l_VertexInputStateCreateInfo{};
		l_VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		l_VertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		l_VertexInputStateCreateInfo.pVertexBindingDescriptions = &l_BindingDescription;
		l_VertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
		l_VertexInputStateCreateInfo.pVertexAttributeDescriptions = l_AttributeDescription;

		VkPipelineInputAssemblyStateCreateInfo l_InputAssemblyStateCreateInfo{};
		l_InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo l_ViewportStateCreateInfo{};
		l_ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_ViewportStateCreateInfo.viewportCount = 1;
		l_ViewportStateCreateInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_RasterizationStateCreateInfo{};
		l_RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		l_RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
		l_RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_RasterizationStateCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo l_MultisampleStateCreateInfo{};
		l_MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_DepthStencilStateCreateInfo{};
		l_DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_DepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
		l_DepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState l_ColorBlendAttachmentState{};
		l_ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		l_ColorBlendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo l_ColorBlendStateCreateInfo{};
		l_ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_ColorBlendStateCreateInfo.attachmentCount = 1;
		l_ColorBlendStateCreateInfo.pAttachments = &l_ColorBlendAttachmentState;

		const VkDynamicState l_DynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo l_DynamicStateCreateInfo{};
		l_DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_DynamicStateCreateInfo.dynamicStateCount = (uint32_t)std::size(l_DynamicState);
		l_DynamicStateCreateInfo.pDynamicStates = l_DynamicState;

		// Push constants
		VkPushConstantRange l_PushConstantRange{};
		l_PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		l_PushConstantRange.offset = 0;
		l_PushConstantRange.size = static_cast<uint32_t>(sizeof(SimplePushConstants));

		VkPipelineLayoutCreateInfo l_PipelineLayoutCreateInfo{};
		l_PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		l_PipelineLayoutCreateInfo.pPushConstantRanges = &l_PushConstantRange;

		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_PipelineLayoutCreateInfo, m_Allocator, &m_PipelineLayout), "Failed vkCreatePipelineLayout");

		VkPipelineRenderingCreateInfo l_RenderingCreateInfo{};
		l_RenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_RenderingCreateInfo.colorAttachmentCount = 1;
		l_RenderingCreateInfo.pColorAttachmentFormats = &m_ColorFormat;
		l_RenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		l_RenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VkGraphicsPipelineCreateInfo l_GraphicsPipelineCreateInfo{};
		l_GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_GraphicsPipelineCreateInfo.pNext = &l_RenderingCreateInfo;
		l_GraphicsPipelineCreateInfo.stageCount = 2;
		l_GraphicsPipelineCreateInfo.pStages = l_ShaderStageCreateInfo;
		l_GraphicsPipelineCreateInfo.pVertexInputState = &l_VertexInputStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pInputAssemblyState = &l_InputAssemblyStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pViewportState = &l_ViewportStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pRasterizationState = &l_RasterizationStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pMultisampleState = &l_MultisampleStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pDepthStencilState = &l_DepthStencilStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pColorBlendState = &l_ColorBlendStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pDynamicState = &l_DynamicStateCreateInfo;
		l_GraphicsPipelineCreateInfo.layout = m_PipelineLayout;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_GraphicsPipelineCreateInfo, m_Allocator, &m_Pipeline), "Failed vkCreateGraphicsPipelines");

		vkDestroyShaderModule(m_Device, l_VertexShaderModule, m_Allocator);
		vkDestroyShaderModule(m_Device, l_FragmentShaderModule, m_Allocator);
	}

	void VulkanPipeline::DestroyPipeline()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_Device, m_Pipeline, m_Allocator);
			m_Pipeline = VK_NULL_HANDLE;
		}

		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_Device, m_PipelineLayout, m_Allocator);
			m_PipelineLayout = VK_NULL_HANDLE;
		}
	}

	VkShaderModule VulkanPipeline::CreateShaderModule(const std::string& path) const
	{
		const std::vector<char> l_Code = Utilities::FileManagement::LoadFromFile(path);

		if (l_Code.empty() || (l_Code.size() % 4) != 0)
		{
			TR_CORE_CRITICAL("Invalid SPIR-V file (size not multiple of 4): {}", path.c_str());

			std::abort();
		}

		VkShaderModuleCreateInfo l_CreateInfo{};
		l_CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		l_CreateInfo.codeSize = l_Code.size();
		l_CreateInfo.pCode = reinterpret_cast<const uint32_t*>(l_Code.data());

		VkShaderModule l_Module = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_CreateInfo, m_Allocator, &l_Module), "Failed vkCreateShaderModule");

		return l_Module;
	}
}