#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Trinity
{
	class VulkanDevice;
	class VulkanSwapchain;

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass() = default;
		~VulkanRenderPass() = default;

		VulkanRenderPass(const VulkanRenderPass&) = delete;
		VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
		VulkanRenderPass(VulkanRenderPass&&) = delete;
		VulkanRenderPass& operator=(VulkanRenderPass&&) = delete;

		void Initialize(const VulkanDevice& device, const VulkanSwapchain& swapchain, VkAllocationCallbacks* allocator = nullptr);
		void Shutdown();

		void Recreate(const VulkanSwapchain& swapchain);

		VkRenderPass GetRenderPass() const { return m_RenderPass; }

	private:
		void CreateRenderPass(VkFormat colorFormat, VkFormat depthFormat);
		void DestroyRenderPass();

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;
		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
	};
}