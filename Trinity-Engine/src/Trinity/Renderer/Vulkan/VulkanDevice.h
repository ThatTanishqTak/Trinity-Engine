#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace Trinity
{
	class VulkanDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice() = default;

		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;
		VulkanDevice(VulkanDevice&&) = delete;
		VulkanDevice& operator=(VulkanDevice&&) = delete;

		// Must be called after a VkInstance and VkSurfaceKHR exist.
		void Initialize(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks* allocator = nullptr);
		void Shutdown();

		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }

		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }
		VkQueue GetTransferQueue() const { return m_TransferQueue; }
		VkQueue GetComputeQueue() const { return m_ComputeQueue; }

		uint32_t GetGraphicsQueueFamilyIndex() const;
		uint32_t GetPresentQueueFamilyIndex() const;
		uint32_t GetTransferQueueFamilyIndex() const;
		uint32_t GetComputeQueueFamilyIndex() const;

		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
		const VkPhysicalDeviceFeatures& GetFeatures() const { return m_Features; }
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }

		// Convenience helpers for resource allocation.
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
		VkFormat FindDepthFormat() const;

	public:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> GraphicsFamily;
			std::optional<uint32_t> PresentFamily;
			std::optional<uint32_t> TransferFamily;
			std::optional<uint32_t> ComputeFamily;

			bool IsComplete() const
			{
				return GraphicsFamily.has_value() && PresentFamily.has_value();
			}
		};

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR Capabilities{};
			std::vector<VkSurfaceFormatKHR> Formats;
			std::vector<VkPresentModeKHR> PresentModes;
		};

		QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice) const;

	private:
		void PickPhysicalDevice();
		void CreateLogicalDevice();

		bool IsDeviceSuitable(VkPhysicalDevice physicalDevice) const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) const;
		int ScorePhysicalDevice(VkPhysicalDevice physicalDevice) const;
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice) const;

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;
		VkQueue m_TransferQueue = VK_NULL_HANDLE;
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;

		QueueFamilyIndices m_QueueFamilyIndices;

		VkPhysicalDeviceProperties m_Properties{};
		VkPhysicalDeviceFeatures m_Features{};
		VkPhysicalDeviceMemoryProperties m_MemoryProperties{};

		std::vector<const char*> m_DeviceExtensions;
	};
}