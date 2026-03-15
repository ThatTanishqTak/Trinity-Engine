#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"
#include "Trinity/Renderer/Vulkan/VulkanTexture.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <cstring>
#include <array>

namespace Trinity
{
	void VulkanRenderer::InitDeferredResources()
	{
		m_DescriptorAllocator.Initialize(m_Device.GetDevice(), m_Context.GetAllocator(), m_FramesInFlight, s_MaxTextureDescriptorsPerFrame);
		m_WhiteTexture.CreateSolid(m_Allocator, m_UploadContext, m_Device.GetDevice(), m_Context.GetAllocator(), 255, 255, 255, 255);

		{
			VkSamplerCreateInfo l_Info{};
			l_Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			l_Info.magFilter = VK_FILTER_LINEAR;
			l_Info.minFilter = VK_FILTER_LINEAR;
			l_Info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			l_Info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			l_Info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			l_Info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			l_Info.maxLod = 0.0f;
			l_Info.maxAnisotropy = 1.0f;
			Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &m_GBufferSampler), "VulkanRenderer: GBuffer vkCreateSampler failed");
		}

		{
			VkSamplerCreateInfo l_Info{};
			l_Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			l_Info.magFilter = VK_FILTER_LINEAR;
			l_Info.minFilter = VK_FILTER_LINEAR;
			l_Info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			l_Info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			l_Info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			l_Info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			l_Info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			l_Info.compareEnable = VK_TRUE;
			l_Info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			l_Info.maxLod = 0.0f;
			l_Info.maxAnisotropy = 1.0f;
			Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &m_ShadowMapSampler), "VulkanRenderer: shadow vkCreateSampler failed");
		}

		{
			VkImageCreateInfo l_Info{};
			l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			l_Info.imageType = VK_IMAGE_TYPE_2D;
			l_Info.extent = { s_ShadowMapSize, s_ShadowMapSize, 1 };
			l_Info.mipLevels = 1;
			l_Info.arrayLayers = 1;
			l_Info.format = m_SceneViewportDepthFormat;
			l_Info.tiling = VK_IMAGE_TILING_OPTIMAL;
			l_Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_Info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			l_Info.samples = VK_SAMPLE_COUNT_1_BIT;
			l_Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			m_Allocator.CreateImage(l_Info, m_ShadowMapImage, m_ShadowMapAllocation);

			VkImageViewCreateInfo l_ViewInfo{};
			l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			l_ViewInfo.image = m_ShadowMapImage;
			l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			l_ViewInfo.format = m_SceneViewportDepthFormat;
			l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			l_ViewInfo.subresourceRange.levelCount = 1;
			l_ViewInfo.subresourceRange.layerCount = 1;
			Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device.GetDevice(), &l_ViewInfo, m_Context.GetAllocator(), &m_ShadowMapView), "VulkanRenderer: shadow vkCreateImageView failed");

			VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(0);
			m_Command.Begin(0);

			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			l_Barrier.image = m_ShadowMapImage;
			l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			VkDependencyInfo l_Dep{};
			l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_Dep.imageMemoryBarrierCount = 1;
			l_Dep.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(l_Cmd, &l_Dep);

			m_Command.End(0);

			VkSubmitInfo l_Submit{};
			l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			l_Submit.commandBufferCount = 1;
			l_Submit.pCommandBuffers = &l_Cmd;
			vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_Submit, VK_NULL_HANDLE);
			vkQueueWaitIdle(m_Device.GetGraphicsQueue());
		}

		m_GBuffer.Initialize(m_Context, m_Device, m_Allocator, m_SceneViewportWidth, m_SceneViewportHeight);

		m_LightingUBOs.reserve(m_FramesInFlight);
		for (uint32_t i = 0; i < m_FramesInFlight; ++i)
		{
			m_LightingUBOs.emplace_back(m_Allocator, sizeof(LightingUniformData));
		}

		{
			const std::array<VkDescriptorPoolSize, 2> l_Sizes =
			{ {
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * m_FramesInFlight },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * m_FramesInFlight }
			} };

			VkDescriptorPoolCreateInfo l_PoolInfo{};
			l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			l_PoolInfo.maxSets = 2 * m_FramesInFlight;
			l_PoolInfo.poolSizeCount = static_cast<uint32_t>(l_Sizes.size());
			l_PoolInfo.pPoolSizes = l_Sizes.data();
			Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device.GetDevice(), &l_PoolInfo, m_Context.GetAllocator(), &m_LightingDescriptorPool), "VulkanRenderer: vkCreateDescriptorPool failed");
		}

		CreateShadowPipeline();
		CreateGBufferPipeline();
		CreateLightingPipeline();

		TR_CORE_TRACE("VulkanRenderer: deferred resources initialized");
	}

	void VulkanRenderer::ShutdownDeferredResources()
	{
		const VkDevice l_Dev = m_Device.GetDevice();
		if (l_Dev == VK_NULL_HANDLE)
		{
			return;
		}

		m_LightingUBOs.clear();

		DestroyDeferredPipelines();

		if (m_LightingDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(l_Dev, m_LightingDescriptorPool, m_Context.GetAllocator());
			m_LightingDescriptorPool = VK_NULL_HANDLE;
		}

		m_GBuffer.Shutdown();

		if (m_ShadowMapView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(l_Dev, m_ShadowMapView, m_Context.GetAllocator());
			m_ShadowMapView = VK_NULL_HANDLE;
		}

		if (m_ShadowMapImage != VK_NULL_HANDLE)
		{
			m_Allocator.DestroyImage(m_ShadowMapImage, m_ShadowMapAllocation);
			m_ShadowMapImage = VK_NULL_HANDLE;
			m_ShadowMapAllocation = VK_NULL_HANDLE;
		}

		if (m_ShadowMapSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(l_Dev, m_ShadowMapSampler, m_Context.GetAllocator());
			m_ShadowMapSampler = VK_NULL_HANDLE;
		}

		if (m_GBufferSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(l_Dev, m_GBufferSampler, m_Context.GetAllocator());
			m_GBufferSampler = VK_NULL_HANDLE;
		}

		m_WhiteTexture.Destroy();
		m_DescriptorAllocator.Shutdown();
	}

	void VulkanRenderer::RecreateDeferredResources()
	{
		if (m_SceneViewportWidth == 0 || m_SceneViewportHeight == 0)
		{
			return;
		}

		m_GBuffer.Recreate(m_SceneViewportWidth, m_SceneViewportHeight);
	}

	void VulkanRenderer::CreateShadowPipeline()
	{
		m_ShaderLibrary.Load("Shadow.vert", "Assets/Shaders/Shadow.vert.spv", ShaderStage::Vertex);
		m_ShaderLibrary.Load("Shadow.frag", "Assets/Shaders/Shadow.frag.spv", ShaderStage::Fragment);

		const std::vector<uint32_t>& l_VSSpv = *m_ShaderLibrary.GetSpirV("Shadow.vert");
		const std::vector<uint32_t>& l_FSSpv = *m_ShaderLibrary.GetSpirV("Shadow.frag");

		VkDescriptorSetLayoutCreateInfo l_EmptySetLayout{};
		l_EmptySetLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_EmptySetLayout, m_Context.GetAllocator(), &m_ShadowTextureSetLayout), "Shadow: vkCreateDescriptorSetLayout failed");

		VkPushConstantRange l_PC{};
		l_PC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		l_PC.offset = 0;
		l_PC.size = sizeof(ShadowPushConstants);

		VkPipelineLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_LayoutInfo.setLayoutCount = 1;
		l_LayoutInfo.pSetLayouts = &m_ShadowTextureSetLayout;
		l_LayoutInfo.pushConstantRangeCount = 1;
		l_LayoutInfo.pPushConstantRanges = &l_PC;
		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device.GetDevice(), &l_LayoutInfo, m_Context.GetAllocator(), &m_ShadowPipelineLayout), "Shadow: vkCreatePipelineLayout failed");

		auto l_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
			{
				VkShaderModuleCreateInfo l_Info{};
				l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				l_Info.codeSize = spv.size() * 4;
				l_Info.pCode = spv.data();
				VkShaderModule l_Mod = VK_NULL_HANDLE;
				Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &l_Mod), "vkCreateShaderModule");

				return l_Mod;
			};

		const VkShaderModule l_VS = l_CreateModule(l_VSSpv);
		const VkShaderModule l_FS = l_CreateModule(l_FSSpv);

		VkPipelineShaderStageCreateInfo l_Stages[2]{};
		l_Stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_Stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_Stages[0].module = l_VS;
		l_Stages[0].pName = "main";
		l_Stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_Stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_Stages[1].module = l_FS;
		l_Stages[1].pName = "main";

		VkVertexInputBindingDescription l_Binding{};
		l_Binding.binding = 0;
		l_Binding.stride = sizeof(Geometry::Vertex);
		l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription l_Attrs[3]{};
		l_Attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Position) };
		l_Attrs[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Normal) };
		l_Attrs[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Geometry::Vertex, UV) };

		VkPipelineVertexInputStateCreateInfo l_VI{};
		l_VI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		l_VI.vertexBindingDescriptionCount = 1;
		l_VI.pVertexBindingDescriptions = &l_Binding;
		l_VI.vertexAttributeDescriptionCount = 3;
		l_VI.pVertexAttributeDescriptions = l_Attrs;

		VkPipelineInputAssemblyStateCreateInfo l_IA{};
		l_IA.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_IA.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo l_VP{};
		l_VP.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_VP.viewportCount = 1;
		l_VP.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_RS{};
		l_RS.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_RS.polygonMode = VK_POLYGON_MODE_FILL;
		l_RS.cullMode = VK_CULL_MODE_FRONT_BIT;
		l_RS.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_RS.lineWidth = 1.0f;
		l_RS.depthBiasEnable = VK_TRUE;
		l_RS.depthBiasConstantFactor = 2.0f;
		l_RS.depthBiasSlopeFactor = 2.5f;

		VkPipelineMultisampleStateCreateInfo l_MS{};
		l_MS.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_MS.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_DS{};
		l_DS.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_DS.depthTestEnable = VK_TRUE;
		l_DS.depthWriteEnable = VK_TRUE;
		l_DS.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		VkPipelineColorBlendStateCreateInfo l_CB{};
		l_CB.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		const VkDynamicState l_Dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo l_DynState{};
		l_DynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_DynState.dynamicStateCount = 2;
		l_DynState.pDynamicStates = l_Dyn;

		VkPipelineRenderingCreateInfo l_Rendering{};
		l_Rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_Rendering.depthAttachmentFormat = m_SceneViewportDepthFormat;
		l_Rendering.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VkGraphicsPipelineCreateInfo l_Info{};
		l_Info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_Info.pNext = &l_Rendering;
		l_Info.stageCount = 2;
		l_Info.pStages = l_Stages;
		l_Info.pVertexInputState = &l_VI;
		l_Info.pInputAssemblyState = &l_IA;
		l_Info.pViewportState = &l_VP;
		l_Info.pRasterizationState = &l_RS;
		l_Info.pMultisampleState = &l_MS;
		l_Info.pDepthStencilState = &l_DS;
		l_Info.pColorBlendState = &l_CB;
		l_Info.pDynamicState = &l_DynState;
		l_Info.layout = m_ShadowPipelineLayout;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &l_Info, m_Context.GetAllocator(), &m_ShadowPipeline), "Shadow: vkCreateGraphicsPipelines failed");

		vkDestroyShaderModule(m_Device.GetDevice(), l_VS, m_Context.GetAllocator());
		vkDestroyShaderModule(m_Device.GetDevice(), l_FS, m_Context.GetAllocator());
	}

	void VulkanRenderer::CreateGBufferPipeline()
	{
		m_ShaderLibrary.Load("GBuffer.vert", "Assets/Shaders/GBuffer.vert.spv", ShaderStage::Vertex);
		m_ShaderLibrary.Load("GBuffer.frag", "Assets/Shaders/GBuffer.frag.spv", ShaderStage::Fragment);

		const std::vector<uint32_t>& l_VSSpv = *m_ShaderLibrary.GetSpirV("GBuffer.vert");
		const std::vector<uint32_t>& l_FSSpv = *m_ShaderLibrary.GetSpirV("GBuffer.frag");

		VkDescriptorSetLayoutBinding l_TexBinding{};
		l_TexBinding.binding = 0;
		l_TexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		l_TexBinding.descriptorCount = 1;
		l_TexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo l_SetLayoutInfo{};
		l_SetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		l_SetLayoutInfo.bindingCount = 1;
		l_SetLayoutInfo.pBindings = &l_TexBinding;
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_SetLayoutInfo, m_Context.GetAllocator(), &m_GBufferTextureSetLayout), "GBuffer: vkCreateDescriptorSetLayout failed");

		VkPushConstantRange l_PC{};
		l_PC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		l_PC.offset = 0;
		l_PC.size = sizeof(GeometryBufferPushConstants);

		VkPipelineLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_LayoutInfo.setLayoutCount = 1;
		l_LayoutInfo.pSetLayouts = &m_GBufferTextureSetLayout;
		l_LayoutInfo.pushConstantRangeCount = 1;
		l_LayoutInfo.pPushConstantRanges = &l_PC;
		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device.GetDevice(), &l_LayoutInfo, m_Context.GetAllocator(), &m_GBufferPipelineLayout), "GBuffer: vkCreatePipelineLayout failed");

		auto l_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
			{
				VkShaderModuleCreateInfo l_Info{};
				l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				l_Info.codeSize = spv.size() * 4;
				l_Info.pCode = spv.data();
				VkShaderModule l_Mod = VK_NULL_HANDLE;
				Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &l_Mod), "vkCreateShaderModule");

				return l_Mod;
			};

		const VkShaderModule l_VS = l_CreateModule(l_VSSpv);
		const VkShaderModule l_FS = l_CreateModule(l_FSSpv);

		VkPipelineShaderStageCreateInfo l_Stages[2]{};
		l_Stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,   l_VS, "main" };
		l_Stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, l_FS, "main" };

		VkVertexInputBindingDescription l_Binding{};
		l_Binding.binding = 0;
		l_Binding.stride = sizeof(Geometry::Vertex);
		l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription l_Attrs[3]{};
		l_Attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Position) };
		l_Attrs[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Normal) };
		l_Attrs[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Geometry::Vertex, UV) };

		VkPipelineVertexInputStateCreateInfo l_VI{};
		l_VI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		l_VI.vertexBindingDescriptionCount = 1;
		l_VI.pVertexBindingDescriptions = &l_Binding;
		l_VI.vertexAttributeDescriptionCount = 3;
		l_VI.pVertexAttributeDescriptions = l_Attrs;

		VkPipelineInputAssemblyStateCreateInfo l_IA{};
		l_IA.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_IA.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo l_VP{};
		l_VP.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_VP.viewportCount = l_VP.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_RS{};
		l_RS.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_RS.polygonMode = VK_POLYGON_MODE_FILL;
		l_RS.cullMode = VK_CULL_MODE_NONE;
		l_RS.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_RS.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo l_MS{};
		l_MS.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_MS.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_DS{};
		l_DS.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_DS.depthTestEnable = VK_TRUE;
		l_DS.depthWriteEnable = VK_TRUE;
		l_DS.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		VkPipelineColorBlendAttachmentState l_BlendAtt{};
		l_BlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		const VkPipelineColorBlendAttachmentState l_BlendAtts[3] = { l_BlendAtt, l_BlendAtt, l_BlendAtt };
		VkPipelineColorBlendStateCreateInfo l_CB{};
		l_CB.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_CB.attachmentCount = 3;
		l_CB.pAttachments = l_BlendAtts;

		const VkDynamicState l_Dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo l_DynState{};
		l_DynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_DynState.dynamicStateCount = 2;
		l_DynState.pDynamicStates = l_Dyn;

		const VkFormat l_ColorFmts[3] = { VulkanGeometryBuffer::AlbedoFormat, VulkanGeometryBuffer::NormalFormat, VulkanGeometryBuffer::MaterialFormat };
		VkPipelineRenderingCreateInfo l_Rendering{};
		l_Rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_Rendering.colorAttachmentCount = 3;
		l_Rendering.pColorAttachmentFormats = l_ColorFmts;
		l_Rendering.depthAttachmentFormat = m_SceneViewportDepthFormat;

		VkGraphicsPipelineCreateInfo l_Info{};
		l_Info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_Info.pNext = &l_Rendering;
		l_Info.stageCount = 2;
		l_Info.pStages = l_Stages;
		l_Info.pVertexInputState = &l_VI;
		l_Info.pInputAssemblyState = &l_IA;
		l_Info.pViewportState = &l_VP;
		l_Info.pRasterizationState = &l_RS;
		l_Info.pMultisampleState = &l_MS;
		l_Info.pDepthStencilState = &l_DS;
		l_Info.pColorBlendState = &l_CB;
		l_Info.pDynamicState = &l_DynState;
		l_Info.layout = m_GBufferPipelineLayout;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &l_Info, m_Context.GetAllocator(), &m_GBufferPipeline), "GBuffer: vkCreateGraphicsPipelines failed");

		vkDestroyShaderModule(m_Device.GetDevice(), l_VS, m_Context.GetAllocator());
		vkDestroyShaderModule(m_Device.GetDevice(), l_FS, m_Context.GetAllocator());
	}

	void VulkanRenderer::CreateLightingPipeline()
	{
		m_ShaderLibrary.Load("Lighting.vert", "Assets/Shaders/Lighting.vert.spv", ShaderStage::Vertex);
		m_ShaderLibrary.Load("Lighting.frag", "Assets/Shaders/Lighting.frag.spv", ShaderStage::Fragment);

		const std::vector<uint32_t>& l_VSSpv = *m_ShaderLibrary.GetSpirV("Lighting.vert");
		const std::vector<uint32_t>& l_FSSpv = *m_ShaderLibrary.GetSpirV("Lighting.frag");

		const VkDescriptorType l_CIS = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		VkDescriptorSetLayoutBinding l_GBufBindings[5]{};
		for (uint32_t i = 0; i < 4; ++i)
		{
			l_GBufBindings[i] = { i, l_CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		}
		l_GBufBindings[4] = { 4, l_CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

		VkDescriptorSetLayoutCreateInfo l_GBufSetInfo{};
		l_GBufSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		l_GBufSetInfo.bindingCount = 5;
		l_GBufSetInfo.pBindings = l_GBufBindings;
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_GBufSetInfo, m_Context.GetAllocator(), &m_LightingGBufferSetLayout), "Lighting: vkCreateDescriptorSetLayout (GBuffer set) failed");

		VkDescriptorSetLayoutBinding l_UBOBinding{};
		l_UBOBinding.binding = 0;
		l_UBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_UBOBinding.descriptorCount = 1;
		l_UBOBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo l_UBOSetInfo{};
		l_UBOSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		l_UBOSetInfo.bindingCount = 1;
		l_UBOSetInfo.pBindings = &l_UBOBinding;
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_UBOSetInfo, m_Context.GetAllocator(), &m_LightingUBOSetLayout), "Lighting: vkCreateDescriptorSetLayout (UBO set) failed");

		VkPushConstantRange l_PC{};
		l_PC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		l_PC.offset = 0;
		l_PC.size = sizeof(LightingPushConstants);

		const VkDescriptorSetLayout l_Layouts[2] = { m_LightingGBufferSetLayout, m_LightingUBOSetLayout };
		VkPipelineLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_LayoutInfo.setLayoutCount = 2;
		l_LayoutInfo.pSetLayouts = l_Layouts;
		l_LayoutInfo.pushConstantRangeCount = 1;
		l_LayoutInfo.pPushConstantRanges = &l_PC;
		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device.GetDevice(), &l_LayoutInfo, m_Context.GetAllocator(), &m_LightingPipelineLayout), "Lighting: vkCreatePipelineLayout failed");

		auto l_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
			{
				VkShaderModuleCreateInfo l_Info{};
				l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				l_Info.codeSize = spv.size() * 4;
				l_Info.pCode = spv.data();
				VkShaderModule l_Mod = VK_NULL_HANDLE;
				Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &l_Mod), "vkCreateShaderModule");

				return l_Mod;
			};

		const VkShaderModule l_VS = l_CreateModule(l_VSSpv);
		const VkShaderModule l_FS = l_CreateModule(l_FSSpv);

		VkPipelineShaderStageCreateInfo l_Stages[2]{};
		l_Stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,   l_VS, "main" };
		l_Stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, l_FS, "main" };

		VkPipelineVertexInputStateCreateInfo l_VI{};
		l_VI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo l_IA{};
		l_IA.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_IA.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo l_VP{};
		l_VP.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_VP.viewportCount = l_VP.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_RS{};
		l_RS.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_RS.polygonMode = VK_POLYGON_MODE_FILL;
		l_RS.cullMode = VK_CULL_MODE_NONE;
		l_RS.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_RS.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo l_MS{};
		l_MS.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_MS.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_DS{};
		l_DS.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendAttachmentState l_BlendAtt{};
		l_BlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo l_CB{};
		l_CB.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_CB.attachmentCount = 1;
		l_CB.pAttachments = &l_BlendAtt;

		const VkDynamicState l_Dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo l_DynState{};
		l_DynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_DynState.dynamicStateCount = 2;
		l_DynState.pDynamicStates = l_Dyn;

		const VkFormat l_ColorFmt = m_Swapchain.GetImageFormat();
		VkPipelineRenderingCreateInfo l_Rendering{};
		l_Rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_Rendering.colorAttachmentCount = 1;
		l_Rendering.pColorAttachmentFormats = &l_ColorFmt;

		VkGraphicsPipelineCreateInfo l_Info{};
		l_Info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_Info.pNext = &l_Rendering;
		l_Info.stageCount = 2;
		l_Info.pStages = l_Stages;
		l_Info.pVertexInputState = &l_VI;
		l_Info.pInputAssemblyState = &l_IA;
		l_Info.pViewportState = &l_VP;
		l_Info.pRasterizationState = &l_RS;
		l_Info.pMultisampleState = &l_MS;
		l_Info.pDepthStencilState = &l_DS;
		l_Info.pColorBlendState = &l_CB;
		l_Info.pDynamicState = &l_DynState;
		l_Info.layout = m_LightingPipelineLayout;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &l_Info, m_Context.GetAllocator(), &m_LightingPipeline), "Lighting: vkCreateGraphicsPipelines failed");

		vkDestroyShaderModule(m_Device.GetDevice(), l_VS, m_Context.GetAllocator());
		vkDestroyShaderModule(m_Device.GetDevice(), l_FS, m_Context.GetAllocator());
	}

	void VulkanRenderer::DestroyDeferredPipelines()
	{
		const VkDevice l_Dev = m_Device.GetDevice();
		const VkAllocationCallbacks* l_Alloc = m_Context.GetAllocator();

		auto l_Destroy = [&](VkPipeline& pipe, VkPipelineLayout& layout, VkDescriptorSetLayout& set0, VkDescriptorSetLayout* set1 = nullptr)
			{
				if (pipe != VK_NULL_HANDLE)
				{
					vkDestroyPipeline(l_Dev, pipe, l_Alloc);
					pipe = VK_NULL_HANDLE;
				}
				
				if (layout != VK_NULL_HANDLE)
				{
					vkDestroyPipelineLayout(l_Dev, layout, l_Alloc);
					layout = VK_NULL_HANDLE;
				}

				if (set0 != VK_NULL_HANDLE)
				{
					vkDestroyDescriptorSetLayout(l_Dev, set0, l_Alloc);
					set0 = VK_NULL_HANDLE;
				}

				if (set1 && *set1 != VK_NULL_HANDLE)
				{
					vkDestroyDescriptorSetLayout(l_Dev, *set1, l_Alloc);
					*set1 = VK_NULL_HANDLE;
				}
			};

		l_Destroy(m_ShadowPipeline, m_ShadowPipelineLayout, m_ShadowTextureSetLayout);
		l_Destroy(m_GBufferPipeline, m_GBufferPipelineLayout, m_GBufferTextureSetLayout);
		l_Destroy(m_LightingPipeline, m_LightingPipelineLayout, m_LightingGBufferSetLayout, &m_LightingUBOSetLayout);
	}

	VkDescriptorSet VulkanRenderer::BuildTextureDescriptorSet(VkImageView imageView, VkSampler sampler)
	{
		const VkDescriptorSet l_Set = m_DescriptorAllocator.Allocate(m_CurrentFrameIndex, m_GBufferTextureSetLayout);

		VkDescriptorImageInfo l_ImageInfo{};
		l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		l_ImageInfo.imageView = imageView;
		l_ImageInfo.sampler = sampler;

		VkWriteDescriptorSet l_Write{};
		l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_Write.dstSet = l_Set;
		l_Write.dstBinding = 0;
		l_Write.descriptorCount = 1;
		l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		l_Write.pImageInfo = &l_ImageInfo;

		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &l_Write, 0, nullptr);

		return l_Set;
	}

	VkDescriptorSet VulkanRenderer::BuildGBufferDescriptorSet()
	{
		VkDescriptorSetAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		l_AllocInfo.descriptorPool = m_LightingDescriptorPool;
		l_AllocInfo.descriptorSetCount = 1;
		l_AllocInfo.pSetLayouts = &m_LightingGBufferSetLayout;

		VkDescriptorSet l_Set = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device.GetDevice(), &l_AllocInfo, &l_Set), "VulkanRenderer: vkAllocateDescriptorSets (GBuffer set) failed");

		const VkImageLayout l_SRO = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		const VkImageLayout l_DSR = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo l_Imgs[5]{};
		l_Imgs[0] = { m_GBufferSampler, m_GBuffer.GetAlbedoView(), l_SRO };
		l_Imgs[1] = { m_GBufferSampler, m_GBuffer.GetNormalView(), l_SRO };
		l_Imgs[2] = { m_GBufferSampler, m_GBuffer.GetMaterialView(), l_SRO };
		l_Imgs[3] = { m_GBufferSampler, m_SceneViewportDepthImageView, l_DSR };
		l_Imgs[4] = { m_ShadowMapSampler, m_ShadowMapView, l_DSR };

		VkWriteDescriptorSet l_Writes[5]{};
		for (uint32_t i = 0; i < 5; ++i)
		{
			l_Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			l_Writes[i].dstSet = l_Set;
			l_Writes[i].dstBinding = i;
			l_Writes[i].descriptorCount = 1;
			l_Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			l_Writes[i].pImageInfo = &l_Imgs[i];
		}

		vkUpdateDescriptorSets(m_Device.GetDevice(), 5, l_Writes, 0, nullptr);

		return l_Set;
	}

	void VulkanRenderer::BeginShadowPass(const glm::mat4& lightSpaceMatrix)
	{
		if (!m_FrameBegun)
		{
			return;
		}

		m_CurrentLightSpaceMatrix = lightSpaceMatrix;
		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		{
			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			l_Barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			l_Barrier.image = m_ShadowMapImage;
			l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			VkDependencyInfo l_Dep{};
			l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_Dep.imageMemoryBarrierCount = 1;
			l_Dep.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(l_Cmd, &l_Dep);
		}

		VkRenderingAttachmentInfo l_DepthAtt{};
		l_DepthAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_DepthAtt.imageView = m_ShadowMapView;
		l_DepthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		l_DepthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_DepthAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_DepthAtt.clearValue.depthStencil = { 1.0f, 0 };

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea = { { 0, 0 }, { s_ShadowMapSize, s_ShadowMapSize } };
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.pDepthAttachment = &l_DepthAtt;

		vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);

		VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(s_ShadowMapSize), static_cast<float>(s_ShadowMapSize), 0.0f, 1.0f };
		VkRect2D   l_Scissor{ { 0, 0 }, { s_ShadowMapSize, s_ShadowMapSize } };
		vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

		vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowPipeline);

		m_ShadowPassRecording = true;
	}

	void VulkanRenderer::EndShadowPass()
	{
		if (!m_ShadowPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		vkCmdEndRendering(l_Cmd);

		{
			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			l_Barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			l_Barrier.image = m_ShadowMapImage;
			l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			VkDependencyInfo l_Dep{};
			l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_Dep.imageMemoryBarrierCount = 1;
			l_Dep.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(l_Cmd, &l_Dep);
		}

		m_ShadowPassRecording = false;
	}

	void VulkanRenderer::DrawMeshShadow(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& lightSpaceMVP)
	{
		if (!m_ShadowPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		VkBuffer l_VB = reinterpret_cast<VkBuffer>(vertexBuffer.GetNativeHandle());
		VkBuffer l_IB = reinterpret_cast<VkBuffer>(indexBuffer.GetNativeHandle());
		VkDeviceSize l_Off = 0;

		vkCmdBindVertexBuffers(l_Cmd, 0, 1, &l_VB, &l_Off);
		vkCmdBindIndexBuffer(l_Cmd, l_IB, 0, ToVkIndexType(indexBuffer.GetIndexType()));

		ShadowPushConstants l_PC{};
		l_PC.LightSpaceModelViewProjection = lightSpaceMVP;
		vkCmdPushConstants(l_Cmd, m_ShadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConstants), &l_PC);

		vkCmdDrawIndexed(l_Cmd, indexCount, 1, 0, 0, 0);
	}

	void VulkanRenderer::BeginGeometryPass()
	{
		if (!m_FrameBegun || !m_GBuffer.IsValid())
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		const VkImageSubresourceRange l_ColorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		const VulkanImageTransitionState l_CAW = BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite);

		TransitionImageResource(l_Cmd, m_GBuffer.GetAlbedoImage(), l_ColorRange, l_CAW);
		TransitionImageResource(l_Cmd, m_GBuffer.GetNormalImage(), l_ColorRange, l_CAW);
		TransitionImageResource(l_Cmd, m_GBuffer.GetMaterialImage(), l_ColorRange, l_CAW);
		TransitionImageResource(l_Cmd, m_SceneViewportDepthImage, BuildDepthSubresourceRange(), BuildTransitionState(ImageTransitionPreset::DepthAttachmentWrite));

		VkClearValue l_Clear{};
		VkRenderingAttachmentInfo l_ColorAtts[3]{};
		const VkImageView l_Views[3] = { m_GBuffer.GetAlbedoView(), m_GBuffer.GetNormalView(), m_GBuffer.GetMaterialView() };
		for (uint32_t i = 0; i < 3; ++i)
		{
			l_ColorAtts[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			l_ColorAtts[i].imageView = l_Views[i];
			l_ColorAtts[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			l_ColorAtts[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			l_ColorAtts[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			l_ColorAtts[i].clearValue = l_Clear;
		}

		VkClearValue l_DepthClear{};
		l_DepthClear.depthStencil = { 1.0f, 0 };
		VkRenderingAttachmentInfo l_DepthAtt{};
		l_DepthAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_DepthAtt.imageView = m_SceneViewportDepthImageView;
		l_DepthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		l_DepthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_DepthAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_DepthAtt.clearValue = l_DepthClear;

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea = { { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.colorAttachmentCount = 3;
		l_RenderingInfo.pColorAttachments = l_ColorAtts;
		l_RenderingInfo.pDepthAttachment = &l_DepthAtt;

		vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);

		VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(m_SceneViewportWidth), static_cast<float>(m_SceneViewportHeight), 0.0f, 1.0f };
		VkRect2D   l_Scissor{ { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

		vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipeline);

		m_GeometryPassRecording = true;
	}

	void VulkanRenderer::EndGeometryPass()
	{
		if (!m_GeometryPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		vkCmdEndRendering(l_Cmd);

		const VkImageSubresourceRange l_ColorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		const VulkanImageTransitionState l_SRO = BuildTransitionState(ImageTransitionPreset::ShaderReadOnly);
		TransitionImageResource(l_Cmd, m_GBuffer.GetAlbedoImage(), l_ColorRange, l_SRO);
		TransitionImageResource(l_Cmd, m_GBuffer.GetNormalImage(), l_ColorRange, l_SRO);
		TransitionImageResource(l_Cmd, m_GBuffer.GetMaterialImage(), l_ColorRange, l_SRO);

		m_GeometryPassRecording = false;
	}

	void VulkanRenderer::DrawMeshDeferred(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::mat4& viewProjection, Texture2D* albedoTexture)
	{
		if (!m_GeometryPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		VkImageView l_View = m_WhiteTexture.GetImageView();
		VkSampler   l_Sampler = m_WhiteTexture.GetSampler();

		if (albedoTexture)
		{
			if (const auto* l_VTex = dynamic_cast<VulkanTexture2D*>(albedoTexture))
			{
				l_View = l_VTex->GetImageView();
				l_Sampler = l_VTex->GetSampler();
			}
		}

		const VkDescriptorSet l_TexSet = BuildTextureDescriptorSet(l_View, l_Sampler);
		vkCmdBindDescriptorSets(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipelineLayout, 0, 1, &l_TexSet, 0, nullptr);

		VkBuffer l_VB = reinterpret_cast<VkBuffer>(vertexBuffer.GetNativeHandle());
		VkBuffer l_IB = reinterpret_cast<VkBuffer>(indexBuffer.GetNativeHandle());
		VkDeviceSize l_Off = 0;
		vkCmdBindVertexBuffers(l_Cmd, 0, 1, &l_VB, &l_Off);
		vkCmdBindIndexBuffer(l_Cmd, l_IB, 0, ToVkIndexType(indexBuffer.GetIndexType()));

		GeometryBufferPushConstants l_PC{};
		l_PC.ModelViewProjection = viewProjection * model;
		l_PC.Model = model;
		vkCmdPushConstants(l_Cmd, m_GBufferPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GeometryBufferPushConstants), &l_PC);

		vkCmdDrawIndexed(l_Cmd, indexCount, 1, 0, 0, 0);
	}

	void VulkanRenderer::BeginLightingPass()
	{
		if (!m_FrameBegun)
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		TransitionImageResource(l_Cmd, m_SceneViewportImage, BuildColorSubresourceRange(), BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite));

		VkClearValue l_Clear{};
		VkRenderingAttachmentInfo l_ColorAtt{};
		l_ColorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_ColorAtt.imageView = m_SceneViewportImageView;
		l_ColorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_ColorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_ColorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_ColorAtt.clearValue = l_Clear;

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea = { { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.colorAttachmentCount = 1;
		l_RenderingInfo.pColorAttachments = &l_ColorAtt;

		vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);

		VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(m_SceneViewportWidth), static_cast<float>(m_SceneViewportHeight), 0.0f, 1.0f };
		VkRect2D l_Scissor{ { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

		vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline);

		m_LightingPassRecording = true;
	}

	void VulkanRenderer::EndLightingPass()
	{
		if (!m_LightingPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		vkCmdEndRendering(l_Cmd);
		m_LightingPassRecording = false;
	}

	void VulkanRenderer::UploadLights(const void* lightData, uint32_t byteSize)
	{
		if (m_CurrentFrameIndex >= static_cast<uint32_t>(m_LightingUBOs.size()))
		{
			return;
		}

		m_LightingUBOs[m_CurrentFrameIndex].SetData(lightData, static_cast<uint64_t>(byteSize));
	}

	void VulkanRenderer::DrawLightingQuad(const glm::mat4& invViewProjection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar)
	{
		if (!m_LightingPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		vkResetDescriptorPool(m_Device.GetDevice(), m_LightingDescriptorPool, 0);

		const VkDescriptorSet l_GBufSet = BuildGBufferDescriptorSet();

		VkDescriptorSetAllocateInfo l_UBOAllocInfo{};
		l_UBOAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		l_UBOAllocInfo.descriptorPool = m_LightingDescriptorPool;
		l_UBOAllocInfo.descriptorSetCount = 1;
		l_UBOAllocInfo.pSetLayouts = &m_LightingUBOSetLayout;

		VkDescriptorSet l_UBOSet = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device.GetDevice(), &l_UBOAllocInfo, &l_UBOSet), "VulkanRenderer: vkAllocateDescriptorSets (UBO set) failed");

		const VkDescriptorBufferInfo l_BufInfo = m_LightingUBOs[m_CurrentFrameIndex].GetDescriptorBufferInfo();

		VkWriteDescriptorSet l_UBOWrite{};
		l_UBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_UBOWrite.dstSet = l_UBOSet;
		l_UBOWrite.dstBinding = 0;
		l_UBOWrite.descriptorCount = 1;
		l_UBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_UBOWrite.pBufferInfo = &l_BufInfo;
		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &l_UBOWrite, 0, nullptr);

		const VkDescriptorSet l_Sets[2] = { l_GBufSet, l_UBOSet };
		vkCmdBindDescriptorSets(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipelineLayout, 0, 2, l_Sets, 0, nullptr);

		const VulkanSwapchain::SceneColorPolicy& l_ColorPolicy = m_Swapchain.GetSceneColorPolicy();

		LightingPushConstants l_PC{};
		l_PC.InvViewProjection = invViewProjection;
		l_PC.CameraPosition = glm::vec4(cameraPosition, 1.0f);
		l_PC.ColorOutputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneOutputTransfer);
		l_PC.CameraNear = cameraNear;
		l_PC.CameraFar = cameraFar;
		vkCmdPushConstants(l_Cmd, m_LightingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingPushConstants), &l_PC);

		vkCmdDraw(l_Cmd, 3, 1, 0, 0);
	}
}