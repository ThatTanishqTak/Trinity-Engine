#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanDescriptors.h"
#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>
#include <cstring>

namespace Trinity
{
	void VulkanPipeline::Initialize(const VulkanDevice& device, const VulkanRenderPass& renderPass, const VulkanDescriptors& descriptors, const std::string& vertexSPVPath,
		const std::string& fragmentSpvPath, VkAllocationCallbacks* allocator, bool enablePipelineCache)
	{
		m_Device = device.GetDevice();
		m_Allocator = allocator;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Initialize called with invalid VkDevice");

			std::abort();
		}

		m_VertexSPVPath = vertexSPVPath;
		m_FragmentSPVPath = fragmentSpvPath;

		m_RenderPass = renderPass.GetRenderPass();
		if (m_RenderPass == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Initialize called with invalid VkRenderPass");

			std::abort();
		}

		if (enablePipelineCache)
		{
			CreatePipelineCache();
		}

		CreatePipelineLayout(descriptors);
		CreateGraphicsPipeline(m_RenderPass);

		TR_CORE_TRACE("VulkanPipeline initialized");
	}

	void VulkanPipeline::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_ShaderModules.clear();

			return;
		}

		DestroyGraphicsPipeline();
		DestroyPipelineLayout();
		DestroyPipelineCache();

		for (auto& it_Pair : m_ShaderModules)
		{
			if (it_Pair.second != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(m_Device, it_Pair.second, m_Allocator);
			}
		}
		m_ShaderModules.clear();

		m_RenderPass = VK_NULL_HANDLE;
		m_VertexSPVPath.clear();
		m_FragmentSPVPath.clear();

		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
	}

	void VulkanPipeline::Recreate(const VulkanRenderPass& renderPass)
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Recreate called before Initialize");

			std::abort();
		}

		VkRenderPass l_NewRenderPass = renderPass.GetRenderPass();
		if (l_NewRenderPass == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanPipeline::Recreate called with invalid VkRenderPass");

			std::abort();
		}

		if (l_NewRenderPass != m_RenderPass)
		{
			DestroyGraphicsPipeline();
			m_RenderPass = l_NewRenderPass;
			CreateGraphicsPipeline(m_RenderPass);

			TR_CORE_TRACE("VulkanPipeline rebuilt (render pass changed)");
		}
	}

	void VulkanPipeline::CreatePipelineCache()
	{
		if (m_PipelineCache != VK_NULL_HANDLE)
		{
			return;
		}

		VkPipelineCacheCreateInfo l_Info{};
		l_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineCache(m_Device, &l_Info, m_Allocator, &m_PipelineCache), "Failed vkCreatePipelineCache");
	}

	void VulkanPipeline::DestroyPipelineCache()
	{
		if (m_PipelineCache != VK_NULL_HANDLE)
		{
			vkDestroyPipelineCache(m_Device, m_PipelineCache, m_Allocator);
			m_PipelineCache = VK_NULL_HANDLE;
		}
	}

	void VulkanPipeline::CreatePipelineLayout(const VulkanDescriptors& descriptors)
	{
		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			return;
		}

		VkDescriptorSetLayout l_GlobalLayout = descriptors.GetGlobalSetLayout();

		std::vector<VkDescriptorSetLayout> l_SetLayouts;
		if (l_GlobalLayout != VK_NULL_HANDLE)
		{
			l_SetLayouts.push_back(l_GlobalLayout);
		}

		VkPipelineLayoutCreateInfo l_Info{};
		l_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_Info.setLayoutCount = static_cast<uint32_t>(l_SetLayouts.size());
		l_Info.pSetLayouts = l_SetLayouts.empty() ? nullptr : l_SetLayouts.data();
		l_Info.pushConstantRangeCount = 0;
		l_Info.pPushConstantRanges = nullptr;

		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_Info, m_Allocator, &m_PipelineLayout), "Failed vkCreatePipelineLayout");
	}

	void VulkanPipeline::DestroyPipelineLayout()
	{
		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_Device, m_PipelineLayout, m_Allocator);
			m_PipelineLayout = VK_NULL_HANDLE;
		}
	}

	void VulkanPipeline::CreateGraphicsPipeline(VkRenderPass renderPass)
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			return;
		}

		if (m_PipelineLayout == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanPipeline::CreateGraphicsPipeline called with no pipeline layout");

			std::abort();
		}

		if (m_VertexSPVPath.empty() || m_FragmentSPVPath.empty())
		{
			TR_CORE_CRITICAL("VulkanPipeline shader paths not set (vert='{}', frag='{}')", m_VertexSPVPath, m_FragmentSPVPath);

			std::abort();
		}

		VkShaderModule l_VS = GetOrCreateShaderModule(m_VertexSPVPath);
		VkShaderModule l_FS = GetOrCreateShaderModule(m_FragmentSPVPath);

		VkPipelineShaderStageCreateInfo l_VSStage{};
		l_VSStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_VSStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_VSStage.module = l_VS;
		l_VSStage.pName = "main";

		VkPipelineShaderStageCreateInfo l_FSStage{};
		l_FSStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_FSStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_FSStage.module = l_FS;
		l_FSStage.pName = "main";

		VkPipelineShaderStageCreateInfo l_ShaderStages[] = { l_VSStage, l_FSStage };

		// No vertex buffers: "gl_VertexIndex triangle" pipeline.
		VkPipelineVertexInputStateCreateInfo l_VertexInput{};
		l_VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
		l_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_InputAssembly.topology = m_GraphicsDescription.Topology;
		l_InputAssembly.primitiveRestartEnable = VK_FALSE;

		// Dummy viewport/scissor (dynamic state overrides these)
		VkViewport l_DummyViewport{};
		l_DummyViewport.width = 1.0f;
		l_DummyViewport.height = 1.0f;
		l_DummyViewport.minDepth = 0.0f;
		l_DummyViewport.maxDepth = 1.0f;

		VkRect2D l_DummyScissor{};
		l_DummyScissor.extent = { 1, 1 };

		VkPipelineViewportStateCreateInfo l_ViewportState{};
		l_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_ViewportState.viewportCount = 1;
		l_ViewportState.pViewports = &l_DummyViewport;
		l_ViewportState.scissorCount = 1;
		l_ViewportState.pScissors = &l_DummyScissor;

		VkPipelineRasterizationStateCreateInfo l_Raster{};
		l_Raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_Raster.depthClampEnable = VK_FALSE;
		l_Raster.rasterizerDiscardEnable = VK_FALSE;
		l_Raster.polygonMode = m_GraphicsDescription.PolygonMode;
		l_Raster.cullMode = m_GraphicsDescription.CullMode;
		l_Raster.frontFace = m_GraphicsDescription.FrontFace;
		l_Raster.depthBiasEnable = VK_FALSE;
		l_Raster.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo l_Multisample{};
		l_Multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_Multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
		l_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_DepthStencil.depthTestEnable = m_GraphicsDescription.EnableDepthTest ? VK_TRUE : VK_FALSE;
		l_DepthStencil.depthWriteEnable = m_GraphicsDescription.EnableDepthWrite ? VK_TRUE : VK_FALSE;
		l_DepthStencil.depthCompareOp = m_GraphicsDescription.DepthCompareOp;
		l_DepthStencil.depthBoundsTestEnable = VK_FALSE;
		l_DepthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState l_ColorBlendAttachment{};
		l_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		if (m_GraphicsDescription.EnableBlending)
		{
			l_ColorBlendAttachment.blendEnable = VK_TRUE;
			l_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			l_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			l_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			l_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			l_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			l_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}
		else
		{
			l_ColorBlendAttachment.blendEnable = VK_FALSE;
		}

		VkPipelineColorBlendStateCreateInfo l_ColorBlend{};
		l_ColorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_ColorBlend.logicOpEnable = VK_FALSE;
		l_ColorBlend.attachmentCount = 1;
		l_ColorBlend.pAttachments = &l_ColorBlendAttachment;

		VkPipelineDynamicStateCreateInfo l_Dynamic{};
		l_Dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_Dynamic.dynamicStateCount = static_cast<uint32_t>(m_GraphicsDescription.DynamicStates.size());
		l_Dynamic.pDynamicStates = m_GraphicsDescription.DynamicStates.empty() ? nullptr : m_GraphicsDescription.DynamicStates.data();

		VkGraphicsPipelineCreateInfo l_PipelineInfo{};
		l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_PipelineInfo.stageCount = static_cast<uint32_t>(sizeof(l_ShaderStages) / sizeof(l_ShaderStages[0]));
		l_PipelineInfo.pStages = l_ShaderStages;
		l_PipelineInfo.pVertexInputState = &l_VertexInput;
		l_PipelineInfo.pInputAssemblyState = &l_InputAssembly;
		l_PipelineInfo.pViewportState = &l_ViewportState;
		l_PipelineInfo.pRasterizationState = &l_Raster;
		l_PipelineInfo.pMultisampleState = &l_Multisample;
		l_PipelineInfo.pDepthStencilState = &l_DepthStencil;
		l_PipelineInfo.pColorBlendState = &l_ColorBlend;
		l_PipelineInfo.pDynamicState = &l_Dynamic;
		l_PipelineInfo.layout = m_PipelineLayout;
		l_PipelineInfo.renderPass = renderPass;
		l_PipelineInfo.subpass = 0;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 1, &l_PipelineInfo, m_Allocator, &m_Pipeline), "Failed vkCreateGraphicsPipelines");
	}

	void VulkanPipeline::DestroyGraphicsPipeline()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_Device, m_Pipeline, m_Allocator);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	VkShaderModule VulkanPipeline::GetOrCreateShaderModule(const std::string& SPVPath)
	{
		auto a_Module = m_ShaderModules.find(SPVPath);
		if (a_Module != m_ShaderModules.end())
		{
			return a_Module->second;
		}

		// Uses your existing file loader (and yes, it aborts on failure, so this will too).
		const std::vector<char> l_Bytes = Utilities::FileManagement::LoadFromFile(SPVPath);

		if (l_Bytes.empty())
		{
			TR_CORE_CRITICAL("Shader '{}' loaded empty", SPVPath);

			std::abort();
		}

		if ((l_Bytes.size() % 4) != 0)
		{
			TR_CORE_CRITICAL("Shader '{}' size {} is not a multiple of 4 (invalid SPIR-V)", SPVPath, (uint32_t)l_Bytes.size());

			std::abort();
		}

		// Ensure proper uint32_t alignment for pCode.
		std::vector<uint32_t> l_Code(l_Bytes.size() / 4);
		std::memcpy(l_Code.data(), l_Bytes.data(), l_Bytes.size());

		VkShaderModuleCreateInfo l_Info{};
		l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		l_Info.codeSize = l_Bytes.size();
		l_Info.pCode = l_Code.data();

		VkShaderModule l_Module = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_Info, m_Allocator, &l_Module), "Failed vkCreateShaderModule");

		m_ShaderModules[SPVPath] = l_Module;
		TR_CORE_TRACE("Created shader module '{}'", SPVPath);

		return l_Module;
	}
}