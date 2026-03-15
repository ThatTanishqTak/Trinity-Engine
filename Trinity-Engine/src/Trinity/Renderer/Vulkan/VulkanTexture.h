#pragma once

#include "Trinity/Renderer/Texture.h"
#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <string>

namespace Trinity
{
	class VulkanTexture2D final : public Texture2D
	{
	public:
		VulkanTexture2D() = default;
		~VulkanTexture2D() override;

		VulkanTexture2D(const VulkanTexture2D&) = delete;
		VulkanTexture2D& operator=(const VulkanTexture2D&) = delete;

		void Load(VulkanAllocator& allocator, VulkanUploadContext& uploadContext, VkDevice device, VkAllocationCallbacks* hostAllocator, const std::string& filePath, TextureFormat format = TextureFormat::RGBA8_SRGB,bool generateMips = true);
		void CreateSolid(VulkanAllocator& allocator, VulkanUploadContext& uploadContext, VkDevice device, VkAllocationCallbacks* hostAllocator, uint8_t r, uint8_t g, uint8_t b, uint8_t a, TextureFormat format = TextureFormat::RGBA8_UNORM);

		void Destroy();

		uint32_t GetWidth() const override { return m_Width; }
		uint32_t GetHeight() const override { return m_Height; }
		uint32_t GetMipLevels() const override { return m_MipLevels; }
		bool IsValid() const override { return m_Image != VK_NULL_HANDLE; }

		void* GetImageViewHandle() const override { return reinterpret_cast<void*>(static_cast<uintptr_t>(reinterpret_cast<uint64_t>(m_ImageView))); }
		void* GetSamplerHandle() const override { return reinterpret_cast<void*>(static_cast<uintptr_t>(reinterpret_cast<uint64_t>(m_Sampler))); }

		VkImage GetImage() const { return m_Image; }
		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }
		VkFormat GetVkFormat() const { return m_Format; }

	private:
		void AllocateImage(VulkanAllocator& allocator, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format);

		void FinalizeImageView();
		void FinalizeSampler();

		static VkFormat ToVkFormat(TextureFormat format);
		static uint32_t ComputeMipLevels(uint32_t width, uint32_t height);

	private:
		VulkanAllocator* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_HostAllocator = nullptr;

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_MipLevels = 0;
	};
}