#include "Trinity/Renderer/Vulkan/VulkanFramebuffers.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"

#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanFramebuffers::Initialize(const VulkanDevice& device, const VulkanSwapchain& swapchain, const VulkanRenderPass& renderPass, VkAllocationCallbacks* allocator)
	{
		m_DeviceRef = &device;
		m_Device = device.GetDevice();
		m_Allocator = allocator;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanFramebuffers::Initialize called with invalid VkDevice");
			std::abort();
		}

		const VkExtent2D l_Extent = swapchain.GetExtent();
		m_Extent = l_Extent;

		if (l_Extent.width == 0 || l_Extent.height == 0)
		{
			TR_CORE_TRACE("VulkanFramebuffers::Initialize skipped (swapchain extent is {}x{})", l_Extent.width, l_Extent.height);

			return;
		}

		CreateDepthResources(l_Extent);
		CreateFramebuffers(swapchain, renderPass.GetRenderPass());

		TR_CORE_TRACE("Framebuffers created: {} (extent {}x{})", (uint32_t)m_Framebuffers.size(), m_Extent.width, m_Extent.height);
	}

	void VulkanFramebuffers::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_Framebuffers.clear();
			m_DeviceRef = nullptr;

			return;
		}

		DestroyFramebuffers();
		DestroyDepthResources();

		m_DeviceRef = nullptr;
		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
		m_Extent = {};
	}

	void VulkanFramebuffers::Recreate(const VulkanSwapchain& swapchain, const VulkanRenderPass& renderPass)
	{
		if (m_Device == VK_NULL_HANDLE || m_DeviceRef == nullptr)
		{
			TR_CORE_CRITICAL("VulkanFramebuffers::Recreate called before Initialize");

			std::abort();
		}

		DestroyFramebuffers();
		DestroyDepthResources();

		const VkExtent2D l_Extent = swapchain.GetExtent();
		m_Extent = l_Extent;

		if (l_Extent.width == 0 || l_Extent.height == 0)
		{
			TR_CORE_TRACE("VulkanFramebuffers::Recreate skipped (swapchain extent is {}x{})", l_Extent.width, l_Extent.height);

			return;
		}

		CreateDepthResources(l_Extent);
		CreateFramebuffers(swapchain, renderPass.GetRenderPass());

		TR_CORE_TRACE("Framebuffers recreated: {} (extent {}x{})", (uint32_t)m_Framebuffers.size(), m_Extent.width, m_Extent.height);
	}

	VkFramebuffer VulkanFramebuffers::GetFramebuffer(uint32_t imageIndex) const
	{
		if (imageIndex >= m_Framebuffers.size())
		{
			TR_CORE_CRITICAL("VulkanFramebuffers::GetFramebuffer invalid imageIndex {}", imageIndex);

			std::abort();
		}

		return m_Framebuffers[imageIndex];
	}

	void VulkanFramebuffers::CreateDepthResources(VkExtent2D extent)
	{
		if (m_DeviceRef == nullptr)
		{
			TR_CORE_CRITICAL("VulkanFramebuffers::CreateDepthResources missing device reference");

			std::abort();
		}

		m_DepthFormat = m_DeviceRef->FindDepthFormat();
		if (m_DepthFormat == VK_FORMAT_UNDEFINED)
		{
			TR_CORE_CRITICAL("VulkanFramebuffers: failed to select a depth format");

			std::abort();
		}

		VkImageCreateInfo l_ImageInfo{};
		l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
		l_ImageInfo.extent.width = extent.width;
		l_ImageInfo.extent.height = extent.height;
		l_ImageInfo.extent.depth = 1;
		l_ImageInfo.mipLevels = 1;
		l_ImageInfo.arrayLayers = 1;
		l_ImageInfo.format = m_DepthFormat;
		l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		Utilities::VulkanUtilities::VKCheck(vkCreateImage(m_Device, &l_ImageInfo, m_Allocator, &m_DepthImage), "Failed vkCreateImage (depth)");

		VkMemoryRequirements l_MemReq{};
		vkGetImageMemoryRequirements(m_Device, m_DepthImage, &l_MemReq);

		const uint32_t l_MemoryType = m_DeviceRef->FindMemoryType(l_MemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (l_MemoryType == UINT32_MAX)
		{
			TR_CORE_CRITICAL("VulkanFramebuffers: no suitable memory type for depth image");

			std::abort();
		}

		VkMemoryAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		l_AllocInfo.allocationSize = l_MemReq.size;
		l_AllocInfo.memoryTypeIndex = l_MemoryType;

		Utilities::VulkanUtilities::VKCheck(vkAllocateMemory(m_Device, &l_AllocInfo, m_Allocator, &m_DepthMemory), "Failed vkAllocateMemory (depth)");
		Utilities::VulkanUtilities::VKCheck(vkBindImageMemory(m_Device, m_DepthImage, m_DepthMemory, 0), "Failed vkBindImageMemory (depth)");

		VkImageAspectFlags l_Aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (HasStencilComponent(m_DepthFormat))
		{
			l_Aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkImageViewCreateInfo l_ViewInfo{};
		l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_ViewInfo.image = m_DepthImage;
		l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_ViewInfo.format = m_DepthFormat;
		l_ViewInfo.subresourceRange.aspectMask = l_Aspect;
		l_ViewInfo.subresourceRange.baseMipLevel = 0;
		l_ViewInfo.subresourceRange.levelCount = 1;
		l_ViewInfo.subresourceRange.baseArrayLayer = 0;
		l_ViewInfo.subresourceRange.layerCount = 1;

		Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_Allocator, &m_DepthImageView), "Failed vkCreateImageView (depth)");
	}

	void VulkanFramebuffers::DestroyDepthResources()
	{
		if (m_DepthImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Device, m_DepthImageView, m_Allocator);
			m_DepthImageView = VK_NULL_HANDLE;
		}

		if (m_DepthImage != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_Device, m_DepthImage, m_Allocator);
			m_DepthImage = VK_NULL_HANDLE;
		}

		if (m_DepthMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_Device, m_DepthMemory, m_Allocator);
			m_DepthMemory = VK_NULL_HANDLE;
		}

		m_DepthFormat = VK_FORMAT_UNDEFINED;
	}

	void VulkanFramebuffers::CreateFramebuffers(const VulkanSwapchain& swapchain, VkRenderPass renderPass)
	{
		if (renderPass == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanFramebuffers::CreateFramebuffers called with invalid VkRenderPass");

			std::abort();
		}

		if (m_DepthImageView == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanFramebuffers::CreateFramebuffers called with no depth image view");

			std::abort();
		}

		const std::vector<VkImageView>& l_Views = swapchain.GetImageViews();
		m_Framebuffers.clear();
		m_Framebuffers.resize(l_Views.size(), VK_NULL_HANDLE);

		for (uint32_t i = 0; i < l_Views.size(); ++i)
		{
			VkImageView l_Attachments[] = { l_Views[i], m_DepthImageView };

			VkFramebufferCreateInfo l_FBInfo{};
			l_FBInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			l_FBInfo.renderPass = renderPass;
			l_FBInfo.attachmentCount = 2;
			l_FBInfo.pAttachments = l_Attachments;
			l_FBInfo.width = m_Extent.width;
			l_FBInfo.height = m_Extent.height;
			l_FBInfo.layers = 1;

			Utilities::VulkanUtilities::VKCheck(vkCreateFramebuffer(m_Device, &l_FBInfo, m_Allocator, &m_Framebuffers[i]), "Failed vkCreateFramebuffer");
		}
	}

	void VulkanFramebuffers::DestroyFramebuffers()
	{
		for (VkFramebuffer it_FB : m_Framebuffers)
		{
			if (it_FB != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(m_Device, it_FB, m_Allocator);
			}
		}

		m_Framebuffers.clear();
	}

	bool VulkanFramebuffers::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
}