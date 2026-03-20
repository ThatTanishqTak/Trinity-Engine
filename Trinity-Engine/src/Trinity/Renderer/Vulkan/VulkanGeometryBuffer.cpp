#include "Trinity/Renderer/Vulkan/VulkanGeometryBuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanTexture.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
	// Original overload — delegates to the raw-handle overload.
	void VulkanGeometryBuffer::Initialize(const VulkanContext& context, const VulkanDevice& device, VulkanAllocator& allocator, uint32_t width, uint32_t height)
	{
		Initialize(device.GetDevice(), context.GetAllocator(), allocator, width, height);
	}

	// Raw-handle overload — the real implementation.
	void VulkanGeometryBuffer::Initialize(VkDevice device, VkAllocationCallbacks* hostAllocator, VulkanAllocator& allocator, uint32_t width, uint32_t height)
	{
		m_Allocator = &allocator;
		m_Device = device;
		m_HostAllocator = hostAllocator;
		m_Width = width;
		m_Height = height;

		if (m_Width > 0 && m_Height > 0)
		{
			CreateAttachments();
		}

		TR_CORE_TRACE("VulkanGeometryBuffer Initialized ({}×{})", m_Width, m_Height);
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

		TR_CORE_TRACE("VulkanGeometryBuffer Recreated ({}×{})", m_Width, m_Height);
	}

	void VulkanGeometryBuffer::CreateAttachments()
	{
		constexpr VkImageUsageFlags l_ColorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		CreateAttachment(AlbedoFormat, l_ColorUsage, m_AlbedoImage, m_AlbedoAllocation, m_AlbedoView);
		CreateAttachment(NormalFormat, l_ColorUsage, m_NormalImage, m_NormalAllocation, m_NormalView);
		CreateAttachment(MaterialFormat, l_ColorUsage, m_MaterialImage, m_MaterialAllocation, m_MaterialView);
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

		m_bInitialized = false;
	}

	void VulkanGeometryBuffer::CreateAttachment(TextureFormat format, VkImageUsageFlags usage, VkImage& outImage, VmaAllocation& outAllocation, VkImageView& outView)
	{
		const VkFormat l_VkFormat = TextureFormatToVkFormat(format);
		const VkImageAspectFlags l_Aspect = TextureFormatToAspectFlags(format);

		VkImageCreateInfo l_ImageInfo{};
		l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
		l_ImageInfo.extent.width = m_Width;
		l_ImageInfo.extent.height = m_Height;
		l_ImageInfo.extent.depth = 1;
		l_ImageInfo.mipLevels = 1;
		l_ImageInfo.arrayLayers = 1;
		l_ImageInfo.format = l_VkFormat;
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
		l_ViewInfo.format = l_VkFormat;
		l_ViewInfo.subresourceRange.aspectMask = l_Aspect;
		l_ViewInfo.subresourceRange.baseMipLevel = 0;
		l_ViewInfo.subresourceRange.levelCount = 1;
		l_ViewInfo.subresourceRange.baseArrayLayer = 0;
		l_ViewInfo.subresourceRange.layerCount = 1;

		Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_HostAllocator, &outView), "VulkanGeometryBuffer: vkCreateImageView failed");
	}

	void VulkanGeometryBuffer::TransitionToShaderRead(VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier l_Barrier{};
		l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		l_Barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		l_Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		l_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		const VkImage l_Images[] = { m_AlbedoImage, m_NormalImage, m_MaterialImage };
		for (VkImage it_Image : l_Images)
		{
			l_Barrier.image = it_Image;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &l_Barrier);
		}
	}

	void VulkanGeometryBuffer::TransitionToAttachment(VkCommandBuffer commandBuffer)
	{
		const VkImageLayout l_OldLayout = m_bInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

		m_bInitialized = true;

		VkImageMemoryBarrier l_Barrier{};
		l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		l_Barrier.oldLayout = l_OldLayout;
		l_Barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		l_Barrier.srcAccessMask = m_bInitialized ? VK_ACCESS_SHADER_READ_BIT : 0u;
		l_Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		const VkPipelineStageFlags l_SrcStage = m_bInitialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		const VkImage l_Images[] = { m_AlbedoImage, m_NormalImage, m_MaterialImage };
		for (VkImage it_Image : l_Images)
		{
			l_Barrier.image = it_Image;
			vkCmdPipelineBarrier(commandBuffer, l_SrcStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &l_Barrier);
		}
	}
}