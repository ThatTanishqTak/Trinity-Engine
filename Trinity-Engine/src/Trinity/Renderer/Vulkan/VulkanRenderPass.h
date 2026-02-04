#pragma once

#include "Trinity/Renderer/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
	class VulkanSwapchain;
	class VulkanDevice;

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass() = default;
		~VulkanRenderPass() = default;

		VulkanRenderPass(const VulkanRenderPass&) = delete;
		VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
		VulkanRenderPass(VulkanRenderPass&&) = delete;
		VulkanRenderPass& operator=(VulkanRenderPass&&) = delete;

		void Initialize(const VulkanContext& context, const VulkanSwapchain& swapchain);
		void Shutdown();

		void Recreate(const VulkanSwapchain& swapchain);

		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFormat GetColorFormat() const { return m_ColorFormat; }
		VkFormat GetDepthFormat() const { return m_DepthFormat; }

	private:
		void CreateRenderPass(VkFormat colorFormat, VkFormat depthFormat);
		void DestroyRenderPass();

	private:
		const VulkanDevice* m_DeviceRef = nullptr;

		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;
		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
	};
}