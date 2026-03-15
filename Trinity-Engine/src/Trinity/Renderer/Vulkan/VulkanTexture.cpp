#include "Trinity/Renderer/Vulkan/VulkanTexture.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <stb_image.h>

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace Trinity
{
	VulkanTexture2D::~VulkanTexture2D()
	{
		Destroy();
	}

	void VulkanTexture2D::Load(VulkanAllocator& allocator, VulkanUploadContext& uploadContext,VkDevice device, VkAllocationCallbacks* hostAllocator, const std::string& filePath, TextureFormat format, bool generateMips)
	{
		Destroy();

		m_Allocator = &allocator;
		m_Device = device;
		m_HostAllocator = hostAllocator;
		m_Path = filePath;
		m_Format = ToVkFormat(format);

		int l_Width = 0;
		int l_Height = 0;
		int l_Channels = 0;

		stbi_uc* l_Pixels = stbi_load(filePath.c_str(), &l_Width, &l_Height, &l_Channels, STBI_rgb_alpha);

		if (!l_Pixels)
		{
			TR_CORE_CRITICAL("VulkanTexture2D::Load — stbi_load failed for '{}': {}", filePath.c_str(), stbi_failure_reason());

			std::abort();
		}

		m_Width = static_cast<uint32_t>(l_Width);
		m_Height = static_cast<uint32_t>(l_Height);
		m_MipLevels = generateMips ? ComputeMipLevels(m_Width, m_Height) : 1;

		const VkDeviceSize l_ImageSize = static_cast<VkDeviceSize>(m_Width) * m_Height * 4;

		VkBuffer l_StagingBuffer = VK_NULL_HANDLE;
		VmaAllocation l_StagingAllocation = VK_NULL_HANDLE;

		allocator.CreateBuffer(l_ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, l_StagingBuffer, l_StagingAllocation);

		void* l_MappedData = allocator.MapMemory(l_StagingAllocation);
		std::memcpy(l_MappedData, l_Pixels, static_cast<size_t>(l_ImageSize));
		allocator.UnmapMemory(l_StagingAllocation);

		stbi_image_free(l_Pixels);

		AllocateImage(allocator, m_Width, m_Height, m_MipLevels, m_Format);
		uploadContext.UploadImageWithMips(l_StagingBuffer, m_Image, m_Width, m_Height, m_MipLevels);
		allocator.DestroyBuffer(l_StagingBuffer, l_StagingAllocation);

		FinalizeImageView();
		FinalizeSampler();

		TR_CORE_TRACE("VulkanTexture2D: loaded '{}' ({}x{}, {} mips)", filePath.c_str(), m_Width, m_Height, m_MipLevels);
	}

	void VulkanTexture2D::CreateSolid(VulkanAllocator& allocator, VulkanUploadContext& uploadContext,VkDevice device, VkAllocationCallbacks* hostAllocator, uint8_t r, uint8_t g, uint8_t b, uint8_t a, TextureFormat format)
	{
		Destroy();

		m_Allocator = &allocator;
		m_Device = device;
		m_HostAllocator = hostAllocator;
		m_Format = ToVkFormat(format);
		m_Width = 1;
		m_Height = 1;
		m_MipLevels = 1;

		const uint8_t l_Pixel[4] = { r, g, b, a };
		constexpr VkDeviceSize l_ImageSize = 4;

		VkBuffer l_StagingBuffer = VK_NULL_HANDLE;
		VmaAllocation l_StagingAllocation = VK_NULL_HANDLE;

		allocator.CreateBuffer(l_ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, l_StagingBuffer, l_StagingAllocation);

		void* l_MappedData = allocator.MapMemory(l_StagingAllocation);
		std::memcpy(l_MappedData, l_Pixel, static_cast<size_t>(l_ImageSize));
		allocator.UnmapMemory(l_StagingAllocation);

		AllocateImage(allocator, 1, 1, 1, m_Format);
		uploadContext.UploadImageWithMips(l_StagingBuffer, m_Image, 1, 1, 1);
		allocator.DestroyBuffer(l_StagingBuffer, l_StagingAllocation);

		FinalizeImageView();
		FinalizeSampler();

		TR_CORE_TRACE("VulkanTexture2D: created solid ({},{},{},{}) 1x1", r, g, b, a);
	}

	void VulkanTexture2D::AllocateImage(VulkanAllocator& allocator, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format)
	{
		VkImageCreateInfo l_ImageCreateInfo{};
		l_ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		l_ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		l_ImageCreateInfo.extent.width = width;
		l_ImageCreateInfo.extent.height = height;
		l_ImageCreateInfo.extent.depth = 1;
		l_ImageCreateInfo.mipLevels = mipLevels;
		l_ImageCreateInfo.arrayLayers = 1;
		l_ImageCreateInfo.format = format;
		l_ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		l_ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		l_ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		l_ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		allocator.CreateImage(l_ImageCreateInfo, m_Image, m_Allocation);
	}

	void VulkanTexture2D::FinalizeImageView()
	{
		VkImageViewCreateInfo l_ViewCreateInfo{};
		l_ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_ViewCreateInfo.image = m_Image;
		l_ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_ViewCreateInfo.format = m_Format;
		l_ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		l_ViewCreateInfo.subresourceRange.levelCount = m_MipLevels;
		l_ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		l_ViewCreateInfo.subresourceRange.layerCount = 1;

		Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewCreateInfo, m_HostAllocator, &m_ImageView), "VulkanTexture2D: vkCreateImageView failed");
	}

	void VulkanTexture2D::FinalizeSampler()
	{
		VkSamplerCreateInfo l_SamplerCreateInfo{};
		l_SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		l_SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		l_SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		l_SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		l_SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		l_SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		l_SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		l_SamplerCreateInfo.mipLodBias = 0.0f;
		l_SamplerCreateInfo.anisotropyEnable = VK_FALSE;
		l_SamplerCreateInfo.maxAnisotropy = 1.0f;
		l_SamplerCreateInfo.compareEnable = VK_FALSE;
		l_SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		l_SamplerCreateInfo.minLod = 0.0f;
		l_SamplerCreateInfo.maxLod = static_cast<float>(m_MipLevels);
		l_SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		l_SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device, &l_SamplerCreateInfo, m_HostAllocator, &m_Sampler), "VulkanTexture2D: vkCreateSampler failed");
	}

	void VulkanTexture2D::Destroy()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_Device, m_Sampler, m_HostAllocator);
			m_Sampler = VK_NULL_HANDLE;
		}

		if (m_ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Device, m_ImageView, m_HostAllocator);
			m_ImageView = VK_NULL_HANDLE;
		}

		if (m_Image != VK_NULL_HANDLE && m_Allocator != nullptr)
		{
			m_Allocator->DestroyImage(m_Image, m_Allocation);
			m_Image = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}

		m_Device = VK_NULL_HANDLE;
		m_HostAllocator = nullptr;
		m_Allocator = nullptr;
		m_Format = VK_FORMAT_UNDEFINED;
		m_Width = 0;
		m_Height = 0;
		m_MipLevels = 0;
		m_Path.clear();
	}

	VkFormat VulkanTexture2D::ToVkFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGBA8_SRGB:  return VK_FORMAT_R8G8B8A8_SRGB;
		case TextureFormat::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		default:                         return VK_FORMAT_R8G8B8A8_SRGB;
		}
	}

	uint32_t VulkanTexture2D::ComputeMipLevels(uint32_t width, uint32_t height)
	{
		return static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(std::max(width, height))))) + 1;
	}
}