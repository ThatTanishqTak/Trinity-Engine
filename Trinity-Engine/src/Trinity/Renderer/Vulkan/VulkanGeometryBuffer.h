#pragma once

#include "Trinity/Renderer/Texture.h"
#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanGeometryBuffer
	{
	public:
		static constexpr TextureFormat AlbedoFormat = TextureFormat::RGBA8_SRGB;
		static constexpr TextureFormat NormalFormat = TextureFormat::RGBA16F;
		static constexpr TextureFormat MaterialFormat = TextureFormat::RGBA8_UNORM;

	public:
		void Initialize(const VulkanContext& context, const VulkanDevice& device, VulkanAllocator& allocator, uint32_t width, uint32_t height);
		void Shutdown();
		void Recreate(uint32_t width, uint32_t height);

		bool IsValid() const { return m_AlbedoImage != VK_NULL_HANDLE; }
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		VkImage GetAlbedoImage() const { return m_AlbedoImage; }
		VkImageView GetAlbedoView() const { return m_AlbedoView; }
		VkImage GetNormalImage() const { return m_NormalImage; }
		VkImageView GetNormalView() const { return m_NormalView; }
		VkImage GetMaterialImage() const { return m_MaterialImage; }
		VkImageView GetMaterialView() const { return m_MaterialView; }

		void TransitionToShaderRead(VkCommandBuffer commandBuffer);
		void TransitionToAttachment(VkCommandBuffer commandBuffer);

	private:
		void CreateAttachments();
		void DestroyAttachments();

		void CreateAttachment(TextureFormat format, VkImageUsageFlags usage, VkImage& outImage, VmaAllocation& outAllocation, VkImageView& outView);

	private:
		VulkanAllocator* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_HostAllocator = nullptr;

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		VkImage m_AlbedoImage = VK_NULL_HANDLE;
		VmaAllocation m_AlbedoAllocation = VK_NULL_HANDLE;
		VkImageView m_AlbedoView = VK_NULL_HANDLE;

		VkImage m_NormalImage = VK_NULL_HANDLE;
		VmaAllocation m_NormalAllocation = VK_NULL_HANDLE;
		VkImageView m_NormalView = VK_NULL_HANDLE;

		VkImage m_MaterialImage = VK_NULL_HANDLE;
		VmaAllocation m_MaterialAllocation = VK_NULL_HANDLE;
		VkImageView m_MaterialView = VK_NULL_HANDLE;

		bool m_bInitialized = false;
	};
}