#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanRenderPass::Initialize(const VulkanContext& context, const VulkanSwapchain& swapchain)
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanRenderPass::Initialize called twice (Shutdown first)");

			std::abort();
		}

		m_DeviceRef = context.DeviceRef;
		m_Device = context.Device;
		m_Allocator = context.Allocator;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanRenderPass::Initialize called with invalid VkDevice");

			std::abort();
		}

		if (m_DeviceRef == nullptr)
		{
			TR_CORE_CRITICAL("VulkanRenderPass::Initialize requires VulkanContext.DeviceRef");

			std::abort();
		}

		m_ColorFormat = swapchain.GetImageFormat();

		m_DepthFormat = m_DeviceRef->FindDepthFormat();
		if (m_DepthFormat == VK_FORMAT_UNDEFINED)
		{
			TR_CORE_CRITICAL("VulkanRenderPass::Initialize failed to select a depth format");

			std::abort();
		}

		CreateRenderPass(m_ColorFormat, m_DepthFormat);
	}

	void VulkanRenderPass::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		DestroyRenderPass();

		m_DeviceRef = nullptr;
		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;

		m_ColorFormat = VK_FORMAT_UNDEFINED;
		m_DepthFormat = VK_FORMAT_UNDEFINED;
	}

	void VulkanRenderPass::Recreate(const VulkanSwapchain& swapchain)
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanRenderPass::Recreate called before Initialize");

			std::abort();
		}

		const VkFormat l_NewColorFormat = swapchain.GetImageFormat();
		if (l_NewColorFormat != m_ColorFormat)
		{
			DestroyRenderPass();
			m_ColorFormat = l_NewColorFormat;
			CreateRenderPass(m_ColorFormat, m_DepthFormat);
		}
	}

	void VulkanRenderPass::CreateRenderPass(VkFormat colorFormat, VkFormat depthFormat)
	{
		VkAttachmentDescription l_ColorAttachment{};
		l_ColorAttachment.format = colorFormat;
		l_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		l_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		l_ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		l_ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription l_DepthAttachment{};
		l_DepthAttachment.format = depthFormat;
		l_DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		l_DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		l_DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		l_DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		l_DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference l_ColorRef{};
		l_ColorRef.attachment = 0;
		l_ColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference l_DepthRef{};
		l_DepthRef.attachment = 1;
		l_DepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription l_Subpass{};
		l_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		l_Subpass.colorAttachmentCount = 1;
		l_Subpass.pColorAttachments = &l_ColorRef;
		l_Subpass.pDepthStencilAttachment = &l_DepthRef;

		VkSubpassDependency l_Dependency{};
		l_Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		l_Dependency.dstSubpass = 0;
		l_Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		l_Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		l_Dependency.srcAccessMask = 0;
		l_Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkAttachmentDescription l_Attachments[] = { l_ColorAttachment, l_DepthAttachment };

		VkRenderPassCreateInfo l_CreateInfo{};
		l_CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		l_CreateInfo.attachmentCount = static_cast<uint32_t>(sizeof(l_Attachments) / sizeof(l_Attachments[0]));
		l_CreateInfo.pAttachments = l_Attachments;
		l_CreateInfo.subpassCount = 1;
		l_CreateInfo.pSubpasses = &l_Subpass;
		l_CreateInfo.dependencyCount = 1;
		l_CreateInfo.pDependencies = &l_Dependency;

		Utilities::VulkanUtilities::VKCheck(vkCreateRenderPass(m_Device, &l_CreateInfo, m_Allocator, &m_RenderPass), "Failed vkCreateRenderPass");
	}

	void VulkanRenderPass::DestroyRenderPass()
	{
		if (m_RenderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(m_Device, m_RenderPass, m_Allocator);
			m_RenderPass = VK_NULL_HANDLE;
		}
	}
}