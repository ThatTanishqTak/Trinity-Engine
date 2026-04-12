#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace Trinity
{
	class VulkanInstance;

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;

		bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities{};
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	class VulkanDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice() = default;

		void Initialize(const VulkanInstance& instance, bool enableValidation);
		void Shutdown();

		VkInstance GetInstance() const;
		VkSurfaceKHR GetSurface() const;
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }

		SwapchainSupportDetails QuerySwapchainSupport() const;
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

	private:
		void PickPhysicalDevice();
		void CreateLogicalDevice(bool enableValidation);

		bool IsDeviceSuitable(VkPhysicalDevice device) const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

	private:
		const VulkanInstance* m_Instance = nullptr;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		QueueFamilyIndices m_QueueFamilyIndices;
		VkPhysicalDeviceProperties m_Properties{};
	};
}