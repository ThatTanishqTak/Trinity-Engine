#include "Trinity/Renderer/Vulkan/VulkanGeometryBuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanGeometryBuffer::Initialize(const VulkanContext& context, const VulkanDevice& device, VulkanAllocator& allocator, uint32_t width, uint32_t height)
	{
		m_Allocator = &allocator;
		m_Device = device.GetDevice();
		m_HostAllocator = context.GetAllocator();
		m_Width = width;
		m_Height = height;

		if (m_Width > 0 && m_Height > 0)
		{
			CreateAttachments();
		}

		TR_CORE_TRACE("VulkanGeometryBuffer Initialized ({}x{})", m_Width, m_Height);
	}

	void VulkanGeometryBuffer::Shutdown()
	{
		DestroyAttachments();

		m_Device = VK_NULL_HANDLE;
		m_HostAllocator = nullptr;
		m_Allocator = nullptr;
		m_Width = 0;
		m_Height = 0;

		TR_CORE_TRACE("VulkanGeometryBuffer Shutdown");
	}

	void VulkanGeometryBuffer::Recreate(uint32_t width, uint32_t height)
	{
		if (width == m_Width && height == m_Height && m_AlbedoImage != VK_NULL_HANDLE)
		{
			return;
		}

		DestroyAttachments();

		m_Width = width;
		m_Height = height;

		if (m_Width > 0 && m_Height > 0)
		{
			CreateAttachments();
		}

		TR_CORE_TRACE("VulkanGeometryBuffer Recreated ({}x{})", m_Width, m_Height);
	}

	void VulkanGeometryBuffer::CreateAttachments()
	{
		CreateAttachment(AlbedoFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_AlbedoImage, m_AlbedoAllocation, m_AlbedoView);
		CreateAttachment(NormalFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_NormalImage, m_NormalAllocation, m_NormalView);
		CreateAttachment(MaterialFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_MaterialImage, m_MaterialAllocation, m_MaterialView);
	}

	void VulkanGeometryBuffer::DestroyAttachments()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		auto l_DestroyOne = [&](VkImage& image, VmaAllocation& allocation, VkImageView& view)
			{
				if (view != VK_NULL_HANDLE)
				{
					vkDestroyImageView(m_Device, view, m_HostAllocator);
					view = VK_NULL_HANDLE;
				}

				if (image != VK_NULL_HANDLE)
				{
					m_Allocator->DestroyImage(image, allocation);
					image = VK_NULL_HANDLE;
					allocation = VK_NULL_HANDLE;
				}
			};

		l_DestroyOne(m_AlbedoImage, m_AlbedoAllocation, m_AlbedoView);
		l_DestroyOne(m_NormalImage, m_NormalAllocation, m_NormalView);
		l_DestroyOne(m_MaterialImage, m_MaterialAllocation, m_MaterialView);
	}

	void VulkanGeometryBuffer::CreateAttachment(VkFormat format, VkImageUsageFlags usage, VkImage& outImage, VmaAllocation& outAllocation, VkImageView& outView)
	{
		VkImageCreateInfo l_ImageInfo{};
		l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
		l_ImageInfo.extent.width = m_Width;
		l_ImageInfo.extent.height = m_Height;
		l_ImageInfo.extent.depth = 1;
		l_ImageInfo.mipLevels = 1;
		l_ImageInfo.arrayLayers = 1;
		l_ImageInfo.format = format;
		l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_ImageInfo.usage = usage;
		l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		m_Allocator->CreateImage(l_ImageInfo, outImage, outAllocation);

		VkImageViewCreateInfo l_ViewInfo{};
		l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_ViewInfo.image = outImage;
		l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_ViewInfo.format = format;
		l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_ViewInfo.subresourceRange.baseMipLevel = 0;
		l_ViewInfo.subresourceRange.levelCount = 1;
		l_ViewInfo.subresourceRange.baseArrayLayer = 0;
		l_ViewInfo.subresourceRange.layerCount = 1;

		Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_HostAllocator, &outView), "VulkanGeometryBuffer: vkCreateImageView failed");
	}
}