#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

namespace Trinity
{
	void VulkanSwapchain::Initialize(const VulkanContext& context, const VulkanDevice& device, uint32_t width, uint32_t height)
	{
		TR_CORE_TRACE("Initializing Vulkan Swapchain");

		m_Allocator = context.GetAllocator();
		m_PhysicalDevice = device.GetPhysicalDevice();
		m_Device = device.GetDevice();
		m_Surface = context.GetSurface();

		if (m_PhysicalDevice == VK_NULL_HANDLE || m_Device == VK_NULL_HANDLE || m_Surface == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanSwapchain::Initialize called with invalid Vulkan handles");

			std::abort();
		}

		m_GraphicsQueueFamilyIndex = device.GetGraphicsQueueFamilyIndex();
		m_PresentQueueFamilyIndex = device.GetPresentQueueFamilyIndex();

		// Initialize cached present info with stable defaults.
		m_PresentInfoCached = {};
		m_PresentInfoCached.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		CreateSwapchain(width, height, VK_NULL_HANDLE);

		TR_CORE_TRACE("Vulkan Swapchain Initialized (Images: {})", GetImageCount());
	}

	void VulkanSwapchain::Shutdown()
	{
		TR_CORE_TRACE("Shutting Down Vulkan Swapchain");

		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		DestroySwapchainResources(m_Swapchain, m_ImageViews);

		m_Swapchain = VK_NULL_HANDLE;
		m_Images.clear();
		m_ImageViews.clear();

		m_PhysicalDevice = VK_NULL_HANDLE;
		m_Device = VK_NULL_HANDLE;
		m_Surface = VK_NULL_HANDLE;
		m_Allocator = nullptr;

		TR_CORE_TRACE("Vulkan Swapchain Shutdown Complete");
	}

	void VulkanSwapchain::Recreate(uint32_t width, uint32_t height)
	{
		TR_CORE_TRACE("Recreating Vulkan Swapchain");

		if (width == 0 || height == 0)
		{
			TR_CORE_WARN("Window Minimized");

			return;
		}

		VkSwapchainKHR l_OldSwapchain = m_Swapchain;
		std::vector<VkImageView> l_OldViews = std::move(m_ImageViews);

		CreateSwapchain(width, height, l_OldSwapchain);
		DestroySwapchainResources(l_OldSwapchain, l_OldViews);

		TR_CORE_TRACE("Vulkan Swapchain Recreated (Images: {})", GetImageCount());
	}

	VkResult VulkanSwapchain::AcquireNextImageIndex(VkSemaphore imageAvailableSemaphore, uint32_t& outImageIndex, uint64_t timeout) const
	{
		return vkAcquireNextImageKHR(m_Device, m_Swapchain, timeout, imageAvailableSemaphore, VK_NULL_HANDLE, &outImageIndex);
	}

	const VkPresentInfoKHR& VulkanSwapchain::GetPresentInfo(VkSemaphore waitSemaphore, uint32_t imageIndex) const
	{
		// Cache-backed arrays so vkQueuePresentKHR always has valid pointers.
		m_PresentSwapchainsCached[0] = m_Swapchain;
		m_PresentImageIndicesCached[0] = imageIndex;

		m_PresentInfoCached = {};
		m_PresentInfoCached.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		if (waitSemaphore != VK_NULL_HANDLE)
		{
			m_PresentWaitSemaphoresCached[0] = waitSemaphore;
			m_PresentInfoCached.waitSemaphoreCount = 1;
			m_PresentInfoCached.pWaitSemaphores = m_PresentWaitSemaphoresCached;
		}
		else
		{
			m_PresentInfoCached.waitSemaphoreCount = 0;
			m_PresentInfoCached.pWaitSemaphores = nullptr;
		}

		m_PresentInfoCached.swapchainCount = 1;
		m_PresentInfoCached.pSwapchains = m_PresentSwapchainsCached;
		m_PresentInfoCached.pImageIndices = m_PresentImageIndicesCached;

		// Optional results pointer (useful for debugging / multi-swapchain in future).
		m_PresentResultsCached[0] = VK_SUCCESS;
		m_PresentInfoCached.pResults = m_PresentResultsCached;

		return m_PresentInfoCached;
	}

	VkResult VulkanSwapchain::Present(VkQueue presentQueue, VkSemaphore waitSemaphore, uint32_t imageIndex)
	{
		const VkPresentInfoKHR& l_PresentInfo = GetPresentInfo(waitSemaphore, imageIndex);

		return vkQueuePresentKHR(presentQueue, &l_PresentInfo);
	}

	// -------------------------------------------------------------------------------------------------------------------------

	void VulkanSwapchain::CreateSwapchain(uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain)
	{
		SwapchainSupportDetails l_Support = QuerySwapchainSupport();

		m_SurfaceFormat = ChooseSurfaceFormat(l_Support.Formats);
		m_PresentMode = ChoosePresentMode(l_Support.PresentModes);
		m_Extent = ChooseExtent(l_Support.Capabilities, width, height);

		uint32_t l_ImageCount = l_Support.Capabilities.minImageCount + 1;
		if (l_Support.Capabilities.maxImageCount > 0 && l_ImageCount > l_Support.Capabilities.maxImageCount)
		{
			l_ImageCount = l_Support.Capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR l_CreateInfo{};
		l_CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		l_CreateInfo.surface = m_Surface;
		l_CreateInfo.minImageCount = l_ImageCount;
		l_CreateInfo.imageFormat = m_SurfaceFormat.format;
		l_CreateInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
		l_CreateInfo.imageExtent = m_Extent;
		l_CreateInfo.imageArrayLayers = 1;
		l_CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t l_QueueFamilyIndices[] = { m_GraphicsQueueFamilyIndex, m_PresentQueueFamilyIndex };
		if (m_GraphicsQueueFamilyIndex != m_PresentQueueFamilyIndex)
		{
			l_CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			l_CreateInfo.queueFamilyIndexCount = 2;
			l_CreateInfo.pQueueFamilyIndices = l_QueueFamilyIndices;
		}
		else
		{
			l_CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			l_CreateInfo.queueFamilyIndexCount = 0;
			l_CreateInfo.pQueueFamilyIndices = nullptr;
		}

		l_CreateInfo.preTransform = l_Support.Capabilities.currentTransform;

		VkCompositeAlphaFlagBitsKHR l_Alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		const VkCompositeAlphaFlagsKHR l_SupportedAlpha = l_Support.Capabilities.supportedCompositeAlpha;
		if (l_SupportedAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		{
			l_Alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		}
		else if (l_SupportedAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		{
			l_Alpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
		}
		else if (l_SupportedAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		{
			l_Alpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
		}
		else if (l_SupportedAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
		{
			l_Alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
		}

		l_CreateInfo.compositeAlpha = l_Alpha;
		l_CreateInfo.presentMode = m_PresentMode;
		l_CreateInfo.clipped = VK_TRUE;
		l_CreateInfo.oldSwapchain = oldSwapchain;

		Utilities::VulkanUtilities::VKCheck(vkCreateSwapchainKHR(m_Device, &l_CreateInfo, m_Allocator, &m_Swapchain), "Failed vkCreateSwapchainKHR");

		uint32_t l_ActualImageCount = 0;
		Utilities::VulkanUtilities::VKCheck(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &l_ActualImageCount, nullptr), "Failed vkGetSwapchainImagesKHR");

		m_Images.resize(l_ActualImageCount);
		Utilities::VulkanUtilities::VKCheck(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &l_ActualImageCount, m_Images.data()), "Failed vkGetSwapchainImagesKHR");

		m_ImageViews.resize(l_ActualImageCount);

		for (uint32_t i = 0; i < l_ActualImageCount; i++)
		{
			VkImageViewCreateInfo l_ViewInfo{};
			l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			l_ViewInfo.image = m_Images[i];
			l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			l_ViewInfo.format = m_SurfaceFormat.format;

			l_ViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			l_ViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			l_ViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			l_ViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			l_ViewInfo.subresourceRange.baseMipLevel = 0;
			l_ViewInfo.subresourceRange.levelCount = 1;
			l_ViewInfo.subresourceRange.baseArrayLayer = 0;
			l_ViewInfo.subresourceRange.layerCount = 1;

			Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_Allocator, &m_ImageViews[i]), "Failed vkCreateImageView");
		}
	}

	void VulkanSwapchain::DestroySwapchainResources(VkSwapchainKHR swapchainToDestroy, const std::vector<VkImageView>& viewsToDestroy)
	{
		for (VkImageView it_View : viewsToDestroy)
		{
			if (it_View != VK_NULL_HANDLE)
			{
				vkDestroyImageView(m_Device, it_View, m_Allocator);
			}
		}

		if (swapchainToDestroy != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_Device, swapchainToDestroy, m_Allocator);
		}
	}

	VulkanSwapchain::SwapchainSupportDetails VulkanSwapchain::QuerySwapchainSupport() const
	{
		SwapchainSupportDetails l_Details{};

		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &l_Details.Capabilities),
			"Failed vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

		uint32_t l_FormatCount = 0;
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &l_FormatCount, nullptr),
			"Failed vkGetPhysicalDeviceSurfaceFormatsKHR");

		l_Details.Formats.resize(l_FormatCount);
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &l_FormatCount, l_Details.Formats.data()),
			"Failed vkGetPhysicalDeviceSurfaceFormatsKHR");

		uint32_t l_PresentModeCount = 0;
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &l_PresentModeCount, nullptr),
			"Failed vkGetPhysicalDeviceSurfacePresentModesKHR");

		l_Details.PresentModes.resize(l_PresentModeCount);
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &l_PresentModeCount, l_Details.PresentModes.data()),
			"Failed vkGetPhysicalDeviceSurfacePresentModesKHR");

		return l_Details;
	}

	VkSurfaceFormatKHR VulkanSwapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
	{
		for (const auto& it_Format : formats)
		{
			if (it_Format.format == VK_FORMAT_B8G8R8A8_UNORM && it_Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return it_Format;
			}
		}

		return formats[0];
	}

	VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
	{
		for (VkPresentModeKHR it_Mode : presentModes)
		{
			if (it_Mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return it_Mode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		VkExtent2D l_Extent{};
		l_Extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		l_Extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return l_Extent;
	}
}