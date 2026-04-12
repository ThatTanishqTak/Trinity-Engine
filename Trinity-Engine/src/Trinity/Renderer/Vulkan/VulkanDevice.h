#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace Trinity
{
	class Window;

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

		void Initialize(Window& window, bool enableValidation);
		void Shutdown();

		VkInstance GetInstance() const { return m_Instance; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }
		VkSurfaceKHR GetSurface() const { return m_Surface; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }

		SwapchainSupportDetails QuerySwapchainSupport() const;
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

	private:
		void CreateInstance(bool enableValidation);
		void CreateSurface(Window& window);
		void PickPhysicalDevice();
		void CreateLogicalDevice(bool enableValidation);

		bool IsDeviceSuitable(VkPhysicalDevice device) const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
		bool CheckValidationLayerSupport() const;

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		QueueFamilyIndices m_QueueFamilyIndices;
		VkPhysicalDeviceProperties m_Properties{};
		bool m_ValidationEnabled = false;
	};
}