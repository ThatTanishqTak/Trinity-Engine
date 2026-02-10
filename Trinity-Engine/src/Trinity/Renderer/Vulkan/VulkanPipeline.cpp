#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/FileManagement.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>
#include <vector>

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

		const VkShaderModule l_VertexModule = CreateShaderModule(m_VertexShaderPath);
		const VkShaderModule l_FragmentModule = CreateShaderModule(m_FragmentShaderPath);

		VkPipelineShaderStageCreateInfo l_VertexStage{};
		l_VertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_VertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_VertexStage.module = l_VertexModule;
		l_VertexStage.pName = "main";

		VkPipelineShaderStageCreateInfo l_FragmentStage{};
		l_FragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_FragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_FragmentStage.module = l_FragmentModule;
		l_FragmentStage.pName = "main";

		const VkPipelineShaderStageCreateInfo l_ShaderStages[] = { l_VertexStage, l_FragmentStage };

		VkPipelineVertexInputStateCreateInfo l_VertexInput{};
		l_VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
		l_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		l_InputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo l_ViewportState{};
		l_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_ViewportState.viewportCount = 1;
		l_ViewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_Raster{};
		l_Raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_Raster.depthClampEnable = VK_FALSE;
		l_Raster.rasterizerDiscardEnable = VK_FALSE;
		l_Raster.polygonMode = VK_POLYGON_MODE_FILL;
		l_Raster.cullMode = VK_CULL_MODE_NONE;
		l_Raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_Raster.depthBiasEnable = VK_FALSE;
		l_Raster.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo l_Multisample{};
		l_Multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_Multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
		l_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_DepthStencil.depthTestEnable = VK_FALSE;
		l_DepthStencil.depthWriteEnable = VK_FALSE;
		l_DepthStencil.depthBoundsTestEnable = VK_FALSE;
		l_DepthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState l_ColorAttachment{};
		l_ColorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		l_ColorAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo l_ColorBlend{};
		l_ColorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_ColorBlend.logicOpEnable = VK_FALSE;
		l_ColorBlend.attachmentCount = 1;
		l_ColorBlend.pAttachments = &l_ColorAttachment;

		const VkDynamicState l_DynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo l_DynamicState{};
		l_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_DynamicState.dynamicStateCount = static_cast<uint32_t>(std::size(l_DynamicStates));
		l_DynamicState.pDynamicStates = l_DynamicStates;

		VkPipelineLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_LayoutInfo, m_Allocator, &m_PipelineLayout), "Failed vkCreatePipelineLayout");

		VkPipelineRenderingCreateInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_RenderingInfo.colorAttachmentCount = 1;
		l_RenderingInfo.pColorAttachmentFormats = &m_ColorFormat;
		l_RenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		l_RenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VkGraphicsPipelineCreateInfo l_PipelineInfo{};
		l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_PipelineInfo.pNext = &l_RenderingInfo;
		l_PipelineInfo.stageCount = 2;
		l_PipelineInfo.pStages = l_ShaderStages;
		l_PipelineInfo.pVertexInputState = &l_VertexInput;
		l_PipelineInfo.pInputAssemblyState = &l_InputAssembly;
		l_PipelineInfo.pViewportState = &l_ViewportState;
		l_PipelineInfo.pRasterizationState = &l_Raster;
		l_PipelineInfo.pMultisampleState = &l_Multisample;
		l_PipelineInfo.pDepthStencilState = &l_DepthStencil;
		l_PipelineInfo.pColorBlendState = &l_ColorBlend;
		l_PipelineInfo.pDynamicState = &l_DynamicState;
		l_PipelineInfo.layout = m_PipelineLayout;
		l_PipelineInfo.renderPass = VK_NULL_HANDLE;
		l_PipelineInfo.subpass = 0;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo, m_Allocator, &m_Pipeline), "Failed vkCreateGraphicsPipelines");

		vkDestroyShaderModule(m_Device, l_VertexModule, m_Allocator);
		vkDestroyShaderModule(m_Device, l_FragmentModule, m_Allocator);
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