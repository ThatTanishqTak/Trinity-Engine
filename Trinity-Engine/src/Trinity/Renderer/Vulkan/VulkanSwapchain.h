#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanDevice;

	class VulkanSwapchain
	{
	public:
		VulkanSwapchain() = default;
		~VulkanSwapchain() = default;

		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
		VulkanSwapchain(VulkanSwapchain&&) = delete;
		VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

		void Initialize(const VulkanDevice& device, VkSurfaceKHR surface, VkAllocationCallbacks* allocator = nullptr, bool vsync = true, uint32_t preferredWidth = 0,
			uint32_t preferredHeight = 0);
		void Shutdown();

		bool Recreate(uint32_t preferredWidth = 0, uint32_t preferredHeight = 0);

		VkResult AcquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& outImageIndex);
		VkResult Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore renderFinishedSemaphore);

		VkSwapchainKHR GetSwapchain() const { return m_SwapchainHandle; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkExtent2D GetExtent() const { return m_Extent; }
		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

		const std::vector<VkImage>& GetImages() const { return m_Images; }
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }

		VkImage GetImage(uint32_t index) const;
		VkImageView GetImageView(uint32_t index) const;

		VkImageUsageFlags GetImageUsageFlags() const { return m_ImageUsageFlags; }
		bool IsVSyncEnabled() const { return m_VSync; }

	private:
		void CreateSwapchain(uint32_t preferredWidth, uint32_t preferredHeight, VkSwapchainKHR oldSwapchain);
		void CreateImageViews();

		void DestroyImageViews(const std::vector<VkImageView>& imageViews);
		void DestroySwapchain(VkSwapchainKHR swapchainHandle);

		VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
		VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t preferredWidth, uint32_t preferredHeight) const;
		VkCompositeAlphaFlagBitsKHR ChooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) const;

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkSwapchainKHR m_SwapchainHandle = VK_NULL_HANDLE;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent{};

		VkImageUsageFlags m_ImageUsageFlags = 0;
		bool m_VSync = true;
	};
}