#pragma once

#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanSwapchain
	{
	public:
		enum class ColorOutputPolicy
		{
			SDRLinear,
			SDRsRGB
		};

		void Initialize(const VulkanContext& context, const VulkanDevice& device, uint32_t width, uint32_t height, ColorOutputPolicy colorOutputPolicy);
		void Shutdown();

		void Recreate(uint32_t width, uint32_t height);

		VkSwapchainKHR GetHandle() const { return m_Swapchain; }
		VkExtent2D GetExtent() const { return m_Extent; }
		VkFormat GetImageFormat() const { return m_SurfaceFormat.format; }
		VkPresentModeKHR GetPresentMode() const { return m_PresentMode; }
		SceneColorOutputTransfer GetSceneColorOutputTransfer() const;

		const std::vector<VkImage>& GetImages() const { return m_Images; }
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }

		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

		VkResult AcquireNextImageIndex(VkSemaphore imageAvailableSemaphore, uint32_t& outImageIndex, uint64_t timeout = UINT64_MAX) const;
		VkResult Present(VkQueue presentQueue, VkSemaphore waitSemaphore, uint32_t imageIndex);

		const VkPresentInfoKHR& GetPresentInfo(VkSemaphore waitSemaphore, uint32_t imageIndex) const;

	private:
		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR Capabilities{};
			std::vector<VkSurfaceFormatKHR> Formats{};
			std::vector<VkPresentModeKHR> PresentModes{};
		};

	private:
		void CreateSwapchain(uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain);
		void DestroySwapchainResources(VkSwapchainKHR swapchainToDestroy, const std::vector<VkImageView>& viewsToDestroy);

		SwapchainSupportDetails QuerySwapchainSupport() const;

		VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
		bool TryChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats, VkFormat format, VkColorSpaceKHR colorSpace, VkSurfaceFormatKHR& outSurfaceFormat) const;
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
		VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const;

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		uint32_t m_GraphicsQueueFamilyIndex = 0;
		uint32_t m_PresentQueueFamilyIndex = 0;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkSurfaceFormatKHR m_SurfaceFormat{};
		VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkExtent2D m_Extent{};
		ColorOutputPolicy m_ColorOutputPolicy = ColorOutputPolicy::SDRsRGB;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		mutable VkPresentInfoKHR m_PresentInfoCached{};
		mutable VkSwapchainKHR m_PresentSwapchainsCached[1]{};
		mutable uint32_t m_PresentImageIndicesCached[1]{};
		mutable VkSemaphore m_PresentWaitSemaphoresCached[1]{};
		mutable VkResult m_PresentResultsCached[1]{};
	};
}