#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Utilities.h"

#include <cstring>
#include <vector>

namespace Trinity
{
	struct GlobalUBO
	{
		glm::mat4 Proj = glm::mat4(1.0f);
		glm::mat4 ViewProj = glm::mat4(1.0f);
	};

	struct ObjectPushConstants
	{
		glm::mat4 Model = glm::mat4(1.0f);
	};

	VulkanRenderer::VulkanRenderer() : Renderer(RendererAPI::VULKAN)
	{

	}

	VulkanRenderer::~VulkanRenderer()
	{

	}

	void VulkanRenderer::SetWindow(Window& window)
	{
		m_Window = &window;
	}

	void VulkanRenderer::Initialize()
	{
		if (m_Window == nullptr)
		{
			TR_CORE_CRITICAL("VulkanRenderer::Initialize called without a Window");
			TR_ABORT();
		}

		TR_CORE_TRACE("Initializing vulkan renderer");

		m_Instance.Initialize();
		m_Surface.Initialize(m_Instance, *m_Window);
		m_Device.Initialize(m_Instance.GetInstance(), m_Surface.GetSurface(), m_Instance.GetAllocator());
		m_Context = VulkanContext::Initialize(m_Instance, m_Surface, m_Device);
		m_Swapchain.Initialize(m_Context, m_Window->IsVSync());
		m_RenderPass.Initialize(m_Context, m_Swapchain);
		m_Framebuffers.Initialize(m_Context, m_Swapchain, m_RenderPass);
		m_FrameResources.Initialize(m_FramesInFlight);
		m_Descriptors.Initialize(m_Context, m_FramesInFlight);
		m_GlobalUniformBuffers.resize(m_FramesInFlight);
		m_GlobalUniformMappings.resize(m_FramesInFlight);

		for (uint32_t it_Frame = 0; it_Frame < m_FramesInFlight; ++it_Frame)
		{
			m_GlobalUniformBuffers[it_Frame].Create(m_Device, sizeof(GlobalUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			m_GlobalUniformMappings[it_Frame] = m_GlobalUniformBuffers[it_Frame].Map();
			m_Descriptors.WriteGlobalUBO(it_Frame, m_GlobalUniformBuffers[it_Frame].GetBuffer(), 0, sizeof(GlobalUBO));
		}

		VulkanPipeline::GraphicsDescription l_PipelineDesc{};
		l_PipelineDesc.EnableDepthTest = true;
		l_PipelineDesc.EnableDepthWrite = true;
		l_PipelineDesc.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		l_PipelineDesc.CullMode = VK_CULL_MODE_BACK_BIT;
		l_PipelineDesc.FrontFace = VK_FRONT_FACE_CLOCKWISE;
		l_PipelineDesc.VertexBindings.clear();
		l_PipelineDesc.VertexAttributes.clear();
		m_Pipeline.SetGraphicsDescription(l_PipelineDesc);

		m_Pipeline.Initialize(m_Context, m_RenderPass, m_Descriptors, "Assets/Shaders/Simple.vert.spv", "Assets/Shaders/Simple.frag.spv", true);
		m_Command.Initialize(m_Context, m_FramesInFlight);
		m_Sync.Initialize(m_Context, m_FramesInFlight, m_Swapchain.GetImageCount());

		TR_CORE_TRACE("Vulkan renderer initialized");
	}

	void VulkanRenderer::Shutdown()
	{
		if (m_Context.Device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Context.Device);
		}

		// Destroy renderer-owned meshes
		for (size_t it_Mesh = 0; it_Mesh < m_Meshes.size(); ++it_Mesh)
		{
			if (it_Mesh < m_MeshAlive.size() && m_MeshAlive[it_Mesh])
			{
				m_Meshes[it_Mesh].Destroy();
			}
		}
		m_Meshes.clear();
		m_MeshAlive.clear();

		m_Sync.Shutdown();
		m_Command.Shutdown();

		m_Pipeline.Shutdown();
		m_Descriptors.Shutdown();
		for (size_t it_Frame = 0; it_Frame < m_GlobalUniformBuffers.size(); ++it_Frame)
		{
			if (m_GlobalUniformMappings[it_Frame] != nullptr)
			{
				m_GlobalUniformBuffers[it_Frame].Unmap();
			}
			m_GlobalUniformBuffers[it_Frame].Destroy();
		}
		m_GlobalUniformMappings.clear();
		m_GlobalUniformBuffers.clear();
		m_FrameResources.Shutdown();
		m_Framebuffers.Shutdown();
		m_RenderPass.Shutdown();
		m_Swapchain.Shutdown();
		m_Device.Shutdown();
		m_Surface.Shutdown(m_Instance);
		m_Instance.Shutdown();

		m_Context = {};
	}

	void VulkanRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_PendingWidth = width;
		m_PendingHeight = height;
		m_ResizePending = true;
	}

	void VulkanRenderer::BeginFrame()
	{
		m_FrameBegun = false;
		m_SceneBegun = false;
		m_FrameContext.reset();

		if (m_ResizePending)
		{
			RecreateSwapchain(m_PendingWidth, m_PendingHeight);
			m_ResizePending = false;

			const VkExtent2D l_Extent = m_Swapchain.GetExtent();
			if (l_Extent.width == 0 || l_Extent.height == 0)
			{
				return;
			}
		}

		const uint32_t l_FrameIndex = m_FrameResources.GetCurrentFrameIndex();

		m_Sync.WaitForFrameFence(l_FrameIndex);

		const VkSemaphore l_ImageAvailableSemaphore = m_Sync.GetImageAvailableSemaphore(l_FrameIndex);

		uint32_t l_ImageIndex = 0;
		VkResult l_AcquireResult = m_Swapchain.AcquireNextImage(l_ImageAvailableSemaphore, l_ImageIndex);
		if (l_AcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain(m_PendingWidth, m_PendingHeight);

			return;
		}

		if (l_AcquireResult != VK_SUCCESS && l_AcquireResult != VK_SUBOPTIMAL_KHR)
		{
			TR_CORE_CRITICAL("vkAcquireNextImageKHR failed with {}", (int)l_AcquireResult);
			TR_ABORT();
		}

		const VkFence l_FrameFence = m_Sync.GetInFlightFence(l_FrameIndex);
		const VkFence l_ImageFence = m_Sync.GetImageInFlightFence(l_ImageIndex);

		if (l_ImageFence != VK_NULL_HANDLE && l_ImageFence != l_FrameFence)
		{
			Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Context.Device, 1, &l_ImageFence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences (image in flight)");
		}

		m_Sync.ResetFrameFence(l_FrameIndex);
		m_Sync.SetImageInFlightFence(l_ImageIndex, l_FrameFence);

		VkCommandBuffer l_CommandBuffer = m_Command.BeginFrame(l_FrameIndex);
		m_Command.RecordUploadAcquireBarriers(l_CommandBuffer);
		const VkSemaphore l_RenderFinishedSemaphore = m_Sync.GetRenderFinishedSemaphore(l_ImageIndex);

		FrameContext l_FrameContext{};
		l_FrameContext.FrameIndex = l_FrameIndex;
		l_FrameContext.ImageIndex = l_ImageIndex;
		l_FrameContext.CommandBuffer = l_CommandBuffer;
		l_FrameContext.ImageAvailableSemaphore = l_ImageAvailableSemaphore;
		l_FrameContext.RenderFinishedSemaphore = l_RenderFinishedSemaphore;
		l_FrameContext.InFlightFence = l_FrameFence;

		m_FrameContext = l_FrameContext;
		m_FrameBegun = true;

		VkClearValue l_ClearValues[2]{};

		l_ClearValues[0].color.float32[0] = 0.01f;
		l_ClearValues[0].color.float32[1] = 0.01f;
		l_ClearValues[0].color.float32[2] = 0.01f;
		l_ClearValues[0].color.float32[3] = 1.0f;

		l_ClearValues[1].depthStencil.depth = 1.0f;
		l_ClearValues[1].depthStencil.stencil = 0;

		VkRenderPassBeginInfo l_RPBegin{};
		l_RPBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		l_RPBegin.renderPass = m_RenderPass.GetRenderPass();
		l_RPBegin.framebuffer = m_Framebuffers.GetFramebuffer(l_ImageIndex);
		l_RPBegin.renderArea.offset = { 0, 0 };
		l_RPBegin.renderArea.extent = m_Swapchain.GetExtent();
		l_RPBegin.clearValueCount = 2;
		l_RPBegin.pClearValues = l_ClearValues;

		vkCmdBeginRenderPass(l_CommandBuffer, &l_RPBegin, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport l_Viewport{};
		l_Viewport.x = 0.0f;
		l_Viewport.y = 0.0f;
		l_Viewport.width = static_cast<float>(m_Swapchain.GetExtent().width);
		l_Viewport.height = static_cast<float>(m_Swapchain.GetExtent().height);
		l_Viewport.minDepth = 0.0f;
		l_Viewport.maxDepth = 1.0f;

		VkRect2D l_Scissor{};
		l_Scissor.offset = { 0, 0 };
		l_Scissor.extent = m_Swapchain.GetExtent();

		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);
	}

	void VulkanRenderer::BeginScene(const glm::mat4& viewProjection)
	{
		if (!m_FrameBegun || !m_FrameContext.has_value())
		{
			return;
		}

		if (m_SceneBegun)
		{
			return;
		}

		const uint32_t l_FrameIndex = m_FrameContext->FrameIndex;

		GlobalUBO l_GlobalUBO{};
		l_GlobalUBO.ViewProj = viewProjection;
		std::memcpy(m_GlobalUniformMappings[l_FrameIndex], &l_GlobalUBO, sizeof(GlobalUBO));

		VkCommandBuffer l_CommandBuffer = m_FrameContext->CommandBuffer;

		vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipeline());

		VkDescriptorSet l_GlobalSet = m_Descriptors.GetGlobalSet(l_FrameIndex);
		if (l_GlobalSet != VK_NULL_HANDLE)
		{
			vkCmdBindDescriptorSets(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipelineLayout(), 0, 1, &l_GlobalSet, 0, nullptr);
		}

		m_SceneBegun = true;
	}

	void VulkanRenderer::SubmitMesh(MeshHandle a_Mesh, const glm::mat4& transform)
	{
		if (!m_SceneBegun || !m_FrameContext.has_value())
		{
			return;
		}

		const Geometry::Mesh* l_Mesh = GetMesh(a_Mesh);
		if (l_Mesh == nullptr || l_Mesh->IndexCount == 0)
		{
			return;
		}

		VkCommandBuffer l_CommandBuffer = m_FrameContext->CommandBuffer;

		ObjectPushConstants l_Push{};
		l_Push.Model = transform;

		vkCmdPushConstants(l_CommandBuffer, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(ObjectPushConstants)), &l_Push);

		VkBuffer l_VertexBuffer = l_Mesh->VertexBuffer.GetBuffer();
		VkDeviceSize l_VertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VertexBuffer, l_VertexOffsets);

		vkCmdBindIndexBuffer(l_CommandBuffer, l_Mesh->IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(l_CommandBuffer, l_Mesh->IndexCount, 1, 0, 0, 0);
	}

	void VulkanRenderer::EndScene()
	{
		m_SceneBegun = false;
	}

	void VulkanRenderer::EndFrame()
	{
		if (!m_FrameBegun || !m_FrameContext.has_value())
		{
			return;
		}

		const FrameContext& l_FrameContext = m_FrameContext.value();
		VkCommandBuffer l_CommandBuffer = l_FrameContext.CommandBuffer;

		if (m_SceneBegun)
		{
			EndScene();
		}

		vkCmdEndRenderPass(l_CommandBuffer);
		m_Command.EndFrame(l_FrameContext.FrameIndex);

		const VkSemaphore l_ImageAvailableSemaphore = l_FrameContext.ImageAvailableSemaphore;
		const VkSemaphore l_RenderFinishedSemaphore = l_FrameContext.RenderFinishedSemaphore;
		const VkFence l_InFlightFence = l_FrameContext.InFlightFence;

		std::vector<VulkanCommand::UploadWait> l_UploadWaits;
		m_Command.ConsumeUploadWaits(l_UploadWaits);

		std::vector<VkSemaphore> l_WaitSemaphores;
		std::vector<VkPipelineStageFlags> l_WaitStages;
		l_WaitSemaphores.reserve(1 + l_UploadWaits.size());
		l_WaitStages.reserve(1 + l_UploadWaits.size());

		l_WaitSemaphores.push_back(l_ImageAvailableSemaphore);
		l_WaitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

		for (const VulkanCommand::UploadWait& it_Wait : l_UploadWaits)
		{
			if (it_Wait.Semaphore == VK_NULL_HANDLE)
			{
				continue;
			}

			l_WaitSemaphores.push_back(it_Wait.Semaphore);
			l_WaitStages.push_back(it_Wait.StageMask);
		}

		VkSemaphore l_SignalSemaphores[] = { l_RenderFinishedSemaphore };

		VkSubmitInfo l_SubmitInfo{};
		l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_SubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(l_WaitSemaphores.size());
		l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores.data();
		l_SubmitInfo.pWaitDstStageMask = l_WaitStages.data();
		l_SubmitInfo.commandBufferCount = 1;
		l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;
		l_SubmitInfo.signalSemaphoreCount = 1;
		l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Context.Queues.GraphicsQueue, 1, &l_SubmitInfo, l_InFlightFence), "Failed vkQueueSubmit");

		for (VulkanCommand::UploadWait& it_Wait : l_UploadWaits)
		{
			if (it_Wait.Semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Context.Device, it_Wait.Semaphore, m_Context.Allocator);
				it_Wait.Semaphore = VK_NULL_HANDLE;
			}
		}

		VkResult l_PresentResult = m_Swapchain.Present(m_Context.Queues.PresentQueue, l_FrameContext.ImageIndex, l_RenderFinishedSemaphore);

		if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR || l_PresentResult == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain(m_PendingWidth, m_PendingHeight);
		}
		else if (l_PresentResult != VK_SUCCESS)
		{
			TR_CORE_CRITICAL("vkQueuePresentKHR failed with {}", (int)l_PresentResult);
			TR_ABORT();
		}

		m_FrameResources.AdvanceFrame();
		m_FrameBegun = false;
		m_FrameContext.reset();
	}

	MeshHandle VulkanRenderer::CreateMesh(const std::vector<Geometry::Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (m_Context.Device == VK_NULL_HANDLE)
		{
			return InvalidMeshHandle;
		}

		if (vertices.empty() || indices.empty())
		{
			return InvalidMeshHandle;
		}

		size_t l_Index = SIZE_MAX;

		for (size_t it_Mesh = 0; it_Mesh < m_MeshAlive.size(); ++it_Mesh)
		{
			if (!m_MeshAlive[it_Mesh])
			{
				l_Index = it_Mesh;
				break;
			}
		}

		if (l_Index == SIZE_MAX)
		{
			l_Index = m_Meshes.size();
			m_Meshes.emplace_back();
			m_MeshAlive.push_back(false);
		}

		m_Meshes[l_Index].Upload(m_Device, m_Command, vertices, indices);
		m_MeshAlive[l_Index] = true;

		return static_cast<MeshHandle>(l_Index + 1);
	}

	void VulkanRenderer::DestroyMesh(MeshHandle handle)
	{
		if (handle == InvalidMeshHandle)
		{
			return;
		}

		const size_t l_Index = static_cast<size_t>(handle - 1);
		if (l_Index >= m_Meshes.size() || l_Index >= m_MeshAlive.size())
		{
			return;
		}

		if (!m_MeshAlive[l_Index])
		{
			return;
		}

		m_Meshes[l_Index].Destroy();
		m_MeshAlive[l_Index] = false;
	}

	Geometry::Mesh* VulkanRenderer::GetMeshMutable(MeshHandle handle)
	{
		if (handle == InvalidMeshHandle)
		{
			return nullptr;
		}

		const size_t l_Index = static_cast<size_t>(handle - 1);
		if (l_Index >= m_Meshes.size() || l_Index >= m_MeshAlive.size() || !m_MeshAlive[l_Index])
		{
			return nullptr;
		}

		return &m_Meshes[l_Index];
	}

	const Geometry::Mesh* VulkanRenderer::GetMesh(MeshHandle handle) const
	{
		if (handle == InvalidMeshHandle)
		{
			return nullptr;
		}

		const size_t l_Index = static_cast<size_t>(handle - 1);
		if (l_Index >= m_Meshes.size() || l_Index >= m_MeshAlive.size() || !m_MeshAlive[l_Index])
		{
			return nullptr;
		}

		return &m_Meshes[l_Index];
	}

	void VulkanRenderer::RecreateSwapchain(uint32_t preferredWidth, uint32_t preferredHeight)
	{
		if (m_Context.Device == VK_NULL_HANDLE)
		{
			return;
		}

		vkDeviceWaitIdle(m_Context.Device);

		const bool l_Recreated = m_Swapchain.Recreate(preferredWidth, preferredHeight);
		if (!l_Recreated)
		{
			return;
		}

		m_Sync.RecreateForSwapchain(m_Swapchain.GetImageCount());

		m_RenderPass.Recreate(m_Swapchain);
		m_Framebuffers.Recreate(m_Swapchain, m_RenderPass);
		m_Pipeline.Recreate(m_RenderPass);

		m_FrameResources.Reset();

		m_FrameContext.reset();
		m_FrameBegun = false;
		m_SceneBegun = false;
	}
}