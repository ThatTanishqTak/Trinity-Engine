#pragma once

#include "Trinity/Renderer/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanSwapchain;
	class VulkanRenderPass;
	class VulkanDevice;

	class VulkanFramebuffers
	{
	public:
		VulkanFramebuffers() = default;
		~VulkanFramebuffers() = default;

		VulkanFramebuffers(const VulkanFramebuffers&) = delete;
		VulkanFramebuffers& operator=(const VulkanFramebuffers&) = delete;
		VulkanFramebuffers(VulkanFramebuffers&&) = delete;
		VulkanFramebuffers& operator=(VulkanFramebuffers&&) = delete;

		void Initialize(const VulkanContext& context, const VulkanSwapchain& swapchain, const VulkanRenderPass& renderPass);
		void Shutdown();

		void Recreate(const VulkanSwapchain& swapchain, const VulkanRenderPass& renderPass);

		VkFramebuffer GetFramebuffer(uint32_t imageIndex) const;

		VkFormat GetDepthFormat() const { return m_DepthFormat; }
		VkImageView GetDepthImageView() const { return m_DepthImageView; }

	private:
		void CreateDepthResources(VkExtent2D extent, VkFormat depthFormat);
		void DestroyDepthResources();

		void CreateFramebuffers(const VulkanSwapchain& swapchain, VkRenderPass renderPass);
		void DestroyFramebuffers();

		static bool HasStencilComponent(VkFormat format);

	private:
		const VulkanDevice* m_DeviceRef = nullptr;

		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkExtent2D m_Extent{};

		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
		VkImage m_DepthImage = VK_NULL_HANDLE;
		VkDeviceMemory m_DepthMemory = VK_NULL_HANDLE;
		VkImageView m_DepthImageView = VK_NULL_HANDLE;

		std::vector<VkFramebuffer> m_Framebuffers;
	};
}