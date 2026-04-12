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

		void Initialize(VulkanDevice& device, uint32_t width, uint32_t height, bool vSync);
		void Shutdown();

		void Recreate(uint32_t width, uint32_t height);

		VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkExtent2D GetExtent() const { return m_Extent; }
		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
		VkImageView GetImageView(uint32_t index) const { return m_ImageViews[index]; }

		const std::vector<VkImage>& GetImages() const { return m_Images; }
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }

	private:
		void CreateSwapchain(uint32_t width, uint32_t height);
		void CreateImageViews();
		void CleanupSwapchain();

		VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
		VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const;

	private:
		VulkanDevice* m_Device = nullptr;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent{};
		bool m_VSync = true;
	};
}