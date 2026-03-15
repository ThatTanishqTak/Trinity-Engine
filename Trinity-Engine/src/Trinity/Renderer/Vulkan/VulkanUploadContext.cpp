#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <algorithm>
#include <cstdlib>

namespace Trinity
{
	void VulkanUploadContext::Initialize(const VulkanContext& context, const VulkanDevice& device)
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_WARN("VulkanUploadContext::Initialize called while already initialized. Reinitializing.");

			Shutdown();
		}

		m_Allocator = context.GetAllocator();
		m_Device = device.GetDevice();
		m_Queue = device.GetGraphicsQueue();

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanUploadContext::Initialize - invalid VkDevice");

			std::abort();
		}

		VkCommandPoolCreateInfo l_PoolInfo{};
		l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		l_PoolInfo.queueFamilyIndex = device.GetGraphicsQueueFamilyIndex();
		Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, m_Allocator, &m_Pool), "VulkanUploadContext: vkCreateCommandPool failed");

		VkCommandBufferAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		l_AllocInfo.commandPool = m_Pool;
		l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		l_AllocInfo.commandBufferCount = 1;
		Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &m_CommandBuffer), "VulkanUploadContext: vkAllocateCommandBuffers failed");

		VkFenceCreateInfo l_FenceInfo{};
		l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &m_Fence), "VulkanUploadContext: vkCreateFence failed");

		TR_CORE_TRACE("VulkanUploadContext Initialized");
	}

	void VulkanUploadContext::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		if (m_Fence != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
			vkDestroyFence(m_Device, m_Fence, m_Allocator);
			m_Fence = VK_NULL_HANDLE;
		}

		if (m_Pool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_Device, m_Pool, m_Allocator);
			m_Pool = VK_NULL_HANDLE;
			m_CommandBuffer = VK_NULL_HANDLE;
		}

		m_Queue = VK_NULL_HANDLE;
		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;

		TR_CORE_TRACE("VulkanUploadContext Shutdown");
	}

	void VulkanUploadContext::BeginCommands()
	{
		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX), "VulkanUploadContext: vkWaitForFences failed");
		Utilities::VulkanUtilities::VKCheck(vkResetFences(m_Device, 1, &m_Fence), "VulkanUploadContext: vkResetFences failed");
		Utilities::VulkanUtilities::VKCheck(vkResetCommandPool(m_Device, m_Pool, 0), "VulkanUploadContext: vkResetCommandPool failed");

		VkCommandBufferBeginInfo l_BeginInfo{};
		l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(m_CommandBuffer, &l_BeginInfo), "VulkanUploadContext: vkBeginCommandBuffer failed");
	}

	void VulkanUploadContext::SubmitAndWait()
	{
		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(m_CommandBuffer), "VulkanUploadContext: vkEndCommandBuffer failed");

		VkSubmitInfo l_Submit{};
		l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_Submit.commandBufferCount = 1;
		l_Submit.pCommandBuffers = &m_CommandBuffer;
		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Queue, 1, &l_Submit, m_Fence), "VulkanUploadContext: vkQueueSubmit failed");
		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX), "VulkanUploadContext: vkWaitForFences (post-submit) failed");
	}

	void VulkanUploadContext::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanUploadContext::CopyBuffer called before Initialize");

			std::abort();
		}

		BeginCommands();

		VkBufferCopy l_Region{};
		l_Region.srcOffset = srcOffset;
		l_Region.dstOffset = dstOffset;
		l_Region.size = size;
		vkCmdCopyBuffer(m_CommandBuffer, src, dst, 1, &l_Region);

		SubmitAndWait();
	}

	void VulkanUploadContext::UploadImage(VkBuffer stagingBuffer, VkImage dstImage, uint32_t width, uint32_t height)
	{
		UploadImageWithMips(stagingBuffer, dstImage, width, height, 1);
	}

	void VulkanUploadContext::UploadImageWithMips(VkBuffer stagingBuffer, VkImage dstImage, uint32_t width, uint32_t height, uint32_t mipLevels)
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanUploadContext::UploadImageWithMips called before Initialize");

			std::abort();
		}

		BeginCommands();

		// Transition all mips to TRANSFER_DST_OPTIMAL
		{
			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			l_Barrier.srcAccessMask = VK_ACCESS_2_NONE;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			l_Barrier.image = dstImage;
			l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			l_Barrier.subresourceRange.baseMipLevel = 0;
			l_Barrier.subresourceRange.levelCount = mipLevels;
			l_Barrier.subresourceRange.baseArrayLayer = 0;
			l_Barrier.subresourceRange.layerCount = 1;

			VkDependencyInfo l_Dep{};
			l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_Dep.imageMemoryBarrierCount = 1;
			l_Dep.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(m_CommandBuffer, &l_Dep);
		}

		// Copy mip 0 from staging buffer
		{
			VkBufferImageCopy2 l_CopyRegion{};
			l_CopyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			l_CopyRegion.bufferOffset = 0;
			l_CopyRegion.bufferRowLength = 0;
			l_CopyRegion.bufferImageHeight = 0;
			l_CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			l_CopyRegion.imageSubresource.mipLevel = 0;
			l_CopyRegion.imageSubresource.baseArrayLayer = 0;
			l_CopyRegion.imageSubresource.layerCount = 1;
			l_CopyRegion.imageOffset = { 0, 0, 0 };
			l_CopyRegion.imageExtent = { width, height, 1 };

			VkCopyBufferToImageInfo2 l_CopyInfo{};
			l_CopyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
			l_CopyInfo.srcBuffer = stagingBuffer;
			l_CopyInfo.dstImage = dstImage;
			l_CopyInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			l_CopyInfo.regionCount = 1;
			l_CopyInfo.pRegions = &l_CopyRegion;
			vkCmdCopyBufferToImage2(m_CommandBuffer, &l_CopyInfo);
		}

		// Generate mips via blit
		int32_t l_MipW = static_cast<int32_t>(width);
		int32_t l_MipH = static_cast<int32_t>(height);

		for (uint32_t i = 1; i < mipLevels; ++i)
		{
			// Transition mip i-1: TRANSFER_DST -> TRANSFER_SRC
			{
				VkImageMemoryBarrier2 l_Barrier{};
				l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
				l_Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
				l_Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
				l_Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				l_Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				l_Barrier.image = dstImage;
				l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				l_Barrier.subresourceRange.baseMipLevel = i - 1;
				l_Barrier.subresourceRange.levelCount = 1;
				l_Barrier.subresourceRange.baseArrayLayer = 0;
				l_Barrier.subresourceRange.layerCount = 1;

				VkDependencyInfo l_Dep{};
				l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				l_Dep.imageMemoryBarrierCount = 1;
				l_Dep.pImageMemoryBarriers = &l_Barrier;
				vkCmdPipelineBarrier2(m_CommandBuffer, &l_Dep);
			}

			const int32_t l_DstMipW = std::max(1, l_MipW / 2);
			const int32_t l_DstMipH = std::max(1, l_MipH / 2);

			// Blit mip i-1 -> mip i
			{
				VkImageBlit2 l_Blit{};
				l_Blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
				l_Blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				l_Blit.srcSubresource.mipLevel = i - 1;
				l_Blit.srcSubresource.baseArrayLayer = 0;
				l_Blit.srcSubresource.layerCount = 1;
				l_Blit.srcOffsets[0] = { 0, 0, 0 };
				l_Blit.srcOffsets[1] = { l_MipW, l_MipH, 1 };
				l_Blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				l_Blit.dstSubresource.mipLevel = i;
				l_Blit.dstSubresource.baseArrayLayer = 0;
				l_Blit.dstSubresource.layerCount = 1;
				l_Blit.dstOffsets[0] = { 0, 0, 0 };
				l_Blit.dstOffsets[1] = { l_DstMipW, l_DstMipH, 1 };

				VkBlitImageInfo2 l_BlitInfo{};
				l_BlitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
				l_BlitInfo.srcImage = dstImage;
				l_BlitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				l_BlitInfo.dstImage = dstImage;
				l_BlitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				l_BlitInfo.regionCount = 1;
				l_BlitInfo.pRegions = &l_Blit;
				l_BlitInfo.filter = VK_FILTER_LINEAR;
				vkCmdBlitImage2(m_CommandBuffer, &l_BlitInfo);
			}

			// Transition mip i-1: TRANSFER_SRC -> SHADER_READ_ONLY
			{
				VkImageMemoryBarrier2 l_Barrier{};
				l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
				l_Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
				l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
				l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
				l_Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				l_Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				l_Barrier.image = dstImage;
				l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				l_Barrier.subresourceRange.baseMipLevel = i - 1;
				l_Barrier.subresourceRange.levelCount = 1;
				l_Barrier.subresourceRange.baseArrayLayer = 0;
				l_Barrier.subresourceRange.layerCount = 1;

				VkDependencyInfo l_Dep{};
				l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				l_Dep.imageMemoryBarrierCount = 1;
				l_Dep.pImageMemoryBarriers = &l_Barrier;
				vkCmdPipelineBarrier2(m_CommandBuffer, &l_Dep);
			}

			l_MipW = l_DstMipW;
			l_MipH = l_DstMipH;
		}

		// Transition the last mip: TRANSFER_DST -> SHADER_READ_ONLY
		{
			const VkPipelineStageFlags2 l_SrcStage = (mipLevels > 1) ? VK_PIPELINE_STAGE_2_BLIT_BIT : VK_PIPELINE_STAGE_2_COPY_BIT;

			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = l_SrcStage;
			l_Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			l_Barrier.image = dstImage;
			l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			l_Barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			l_Barrier.subresourceRange.levelCount = 1;
			l_Barrier.subresourceRange.baseArrayLayer = 0;
			l_Barrier.subresourceRange.layerCount = 1;

			VkDependencyInfo l_Dep{};
			l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_Dep.imageMemoryBarrierCount = 1;
			l_Dep.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(m_CommandBuffer, &l_Dep);
		}

		SubmitAndWait();
	}
}