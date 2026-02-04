#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Trinity
{
	class VulkanInstance;
	class VulkanSurface;
	class VulkanDevice;

	struct VulkanQueues
	{
		VkQueue GraphicsQueue = VK_NULL_HANDLE;
		VkQueue PresentQueue = VK_NULL_HANDLE;
		VkQueue TransferQueue = VK_NULL_HANDLE;
		VkQueue ComputeQueue = VK_NULL_HANDLE;

		uint32_t GraphicsFamilyIndex = UINT32_MAX;
		uint32_t PresentFamilyIndex = UINT32_MAX;
		uint32_t TransferFamilyIndex = UINT32_MAX;
		uint32_t ComputeFamilyIndex = UINT32_MAX;
	};

	struct VulkanContext
	{
		VkInstance Instance = VK_NULL_HANDLE;
		VkSurfaceKHR Surface = VK_NULL_HANDLE;

		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VkDevice Device = VK_NULL_HANDLE;

		VkAllocationCallbacks* Allocator = nullptr;

		VulkanQueues Queues{};

		// Optional: gives access to helper functions like FindMemoryType().
		const VulkanDevice* DeviceRef = nullptr;

		static VulkanContext Initialize(VulkanInstance& instance, VulkanSurface& surface, const VulkanDevice& device);
	};
}