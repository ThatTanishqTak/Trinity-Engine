#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Utilities/Utilities.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Trinity
{
	void VulkanSwapchain::Initialize(const VulkanDevice& device, VkSurfaceKHR surface, VkAllocationCallbacks* allocator, bool vsync, uint32_t preferredWidth, uint32_t preferredHeight)
	{
		m_Device = device.GetDevice();
		m_PhysicalDevice = device.GetPhysicalDevice();
		m_Surface = surface;
		m_Allocator = allocator;
		m_VSync = vsync;

		if (m_Device == VK_NULL_HANDLE || m_PhysicalDevice == VK_NULL_HANDLE || m_Surface == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanSwapchain::Initialize called with invalid device/physicalDevice/surface");

			std::abort();
		}

		TR_CORE_TRACE("Creating swapchain");

		CreateSwapchain(preferredWidth, preferredHeight, VK_NULL_HANDLE);
		CreateImageViews();

		TR_CORE_TRACE("Swapchain created: images = {}, format = {}, extent = {}x{}", GetImageCount(), (int)m_ImageFormat, m_Extent.width, m_Extent.height);
	}

	void VulkanSwapchain::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_Images.clear();
			m_ImageViews.clear();

			return;
		}

		TR_CORE_TRACE("Destroying swapchain");
		vkDeviceWaitIdle(m_Device);

		DestroyImageViews(m_ImageViews);
		m_ImageViews.clear();

		DestroySwapchain(m_SwapchainHandle);
		m_SwapchainHandle = VK_NULL_HANDLE;

		m_Images.clear();

		m_ImageFormat = VK_FORMAT_UNDEFINED;
		m_Extent = {};
		m_ImageUsageFlags = 0;

		m_Device = VK_NULL_HANDLE;
		m_PhysicalDevice = VK_NULL_HANDLE;
		m_Surface = VK_NULL_HANDLE;
		m_Allocator = nullptr;
	}

	bool VulkanSwapchain::Recreate(uint32_t preferredWidth, uint32_t preferredHeight)
	{
		if (m_Device == VK_NULL_HANDLE || m_PhysicalDevice == VK_NULL_HANDLE || m_Surface == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanSwapchain::Recreate called before Initialize");

			std::abort();
		}

		VkSurfaceCapabilitiesKHR l_Capabilities{};
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &l_Capabilities), "Failed vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

		const VkExtent2D l_TargetExtent = ChooseExtent(l_Capabilities, preferredWidth, preferredHeight);
		if (l_TargetExtent.width == 0 || l_TargetExtent.height == 0)
		{
			TR_CORE_TRACE("Swapchain recreate skipped: extent is {}x{}", l_TargetExtent.width, l_TargetExtent.height);

			return false;
		}

		TR_CORE_TRACE("Recreating swapchain");

		VkSwapchainKHR l_OldSwapchain = m_SwapchainHandle;
		std::vector<VkImageView> l_OldImageViews = std::move(m_ImageViews);

		m_Images.clear();
		m_ImageViews.clear();

		CreateSwapchain(preferredWidth, preferredHeight, l_OldSwapchain);
		CreateImageViews();

		DestroyImageViews(l_OldImageViews);
		DestroySwapchain(l_OldSwapchain);

		TR_CORE_TRACE("Swapchain recreated: images = {}, extent = {}x{}", GetImageCount(), m_Extent.width, m_Extent.height);

		return true;
	}

	VkResult VulkanSwapchain::AcquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& outImageIndex)
	{
		if (m_SwapchainHandle == VK_NULL_HANDLE)
		{
			return VK_ERROR_INITIALIZATION_FAILED;
		}

		return vkAcquireNextImageKHR(m_Device, m_SwapchainHandle, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &outImageIndex);
	}

	VkResult VulkanSwapchain::Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore renderFinishedSemaphore)
	{
		if (m_SwapchainHandle == VK_NULL_HANDLE)
		{
			return VK_ERROR_INITIALIZATION_FAILED;
		}

		VkPresentInfoKHR l_PresentInfo{};
		l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		l_PresentInfo.waitSemaphoreCount = 1;
		l_PresentInfo.pWaitSemaphores = &renderFinishedSemaphore;
		l_PresentInfo.swapchainCount = 1;
		l_PresentInfo.pSwapchains = &m_SwapchainHandle;
		l_PresentInfo.pImageIndices = &imageIndex;

		return vkQueuePresentKHR(presentQueue, &l_PresentInfo);
	}

	VkImage VulkanSwapchain::GetImage(uint32_t index) const
	{
		if (index >= m_Images.size())
		{
			TR_CORE_CRITICAL("VulkanSwapchain::GetImage invalid index {}", index);

			std::abort();
		}

		return m_Images[index];
	}

	VkImageView VulkanSwapchain::GetImageView(uint32_t index) const
	{
		if (index >= m_ImageViews.size())
		{
			TR_CORE_CRITICAL("VulkanSwapchain::GetImageView invalid index {}", index);

			std::abort();
		}

		return m_ImageViews[index];
	}

	void VulkanSwapchain::CreateSwapchain(uint32_t preferredWidth, uint32_t preferredHeight, VkSwapchainKHR oldSwapchain)
	{
		VkSurfaceCapabilitiesKHR l_Capabilities{};
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &l_Capabilities), "Failed vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

		uint32_t l_FormatCount = 0;
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &l_FormatCount, nullptr), "Failed vkGetPhysicalDeviceSurfaceFormatsKHR (count)");
		if (l_FormatCount == 0)
		{
			TR_CORE_CRITICAL("Swapchain: surface has no formats");

			std::abort();
		}

		std::vector<VkSurfaceFormatKHR> l_Formats(l_FormatCount);
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &l_FormatCount, l_Formats.data()),
			"Failed vkGetPhysicalDeviceSurfaceFormatsKHR");

		uint32_t l_PresentModeCount = 0;
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &l_PresentModeCount, nullptr),
			"Failed vkGetPhysicalDeviceSurfacePresentModesKHR (count)");

		if (l_PresentModeCount == 0)
		{
			TR_CORE_CRITICAL("Swapchain: surface has no present modes");

			std::abort();
		}

		std::vector<VkPresentModeKHR> l_PresentModes(l_PresentModeCount);
		Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &l_PresentModeCount, l_PresentModes.data()),
			"Failed vkGetPhysicalDeviceSurfacePresentModesKHR");

		const VkSurfaceFormatKHR l_SurfaceFormat = ChooseSurfaceFormat(l_Formats);
		const VkPresentModeKHR l_PresentMode = ChoosePresentMode(l_PresentModes);
		const VkExtent2D l_Extent = ChooseExtent(l_Capabilities, preferredWidth, preferredHeight);

		if (l_Extent.width == 0 || l_Extent.height == 0)
		{
			TR_CORE_TRACE("Swapchain create skipped: extent is {}x{}", l_Extent.width, l_Extent.height);
			m_Extent = l_Extent;

			return;
		}

		uint32_t l_ImageCount = l_Capabilities.minImageCount + 1;
		if (l_Capabilities.maxImageCount > 0 && l_ImageCount > l_Capabilities.maxImageCount)
		{
			l_ImageCount = l_Capabilities.maxImageCount;
		}

		// We want to be able to clear the image before you have a render pass.
		VkImageUsageFlags l_RequestedUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		l_RequestedUsage &= l_Capabilities.supportedUsageFlags;
		if ((l_RequestedUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0)
		{
			TR_CORE_CRITICAL("Swapchain: surface does not support COLOR_ATTACHMENT usage");

			std::abort();
		}

		m_ImageUsageFlags = l_RequestedUsage;

		VkSwapchainCreateInfoKHR l_CreateInfo{};
		l_CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		l_CreateInfo.surface = m_Surface;
		l_CreateInfo.minImageCount = l_ImageCount;
		l_CreateInfo.imageFormat = l_SurfaceFormat.format;
		l_CreateInfo.imageColorSpace = l_SurfaceFormat.colorSpace;
		l_CreateInfo.imageExtent = l_Extent;
		l_CreateInfo.imageArrayLayers = 1;
		l_CreateInfo.imageUsage = m_ImageUsageFlags;
		l_CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		l_CreateInfo.queueFamilyIndexCount = 0;
		l_CreateInfo.pQueueFamilyIndices = nullptr;
		l_CreateInfo.preTransform = l_Capabilities.currentTransform;
		l_CreateInfo.compositeAlpha = ChooseCompositeAlpha(l_Capabilities);
		l_CreateInfo.presentMode = l_PresentMode;
		l_CreateInfo.clipped = VK_TRUE;
		l_CreateInfo.oldSwapchain = oldSwapchain;

		Utilities::VulkanUtilities::VKCheck(vkCreateSwapchainKHR(m_Device, &l_CreateInfo, m_Allocator, &m_SwapchainHandle), "Failed vkCreateSwapchainKHR");

		m_ImageFormat = l_SurfaceFormat.format;
		m_Extent = l_Extent;

		uint32_t l_SwapchainImageCount = 0;
		Utilities::VulkanUtilities::VKCheck(vkGetSwapchainImagesKHR(m_Device, m_SwapchainHandle, &l_SwapchainImageCount, nullptr), "Failed vkGetSwapchainImagesKHR (count)");
		m_Images.resize(l_SwapchainImageCount);
		Utilities::VulkanUtilities::VKCheck(vkGetSwapchainImagesKHR(m_Device, m_SwapchainHandle, &l_SwapchainImageCount, m_Images.data()), "Failed vkGetSwapchainImagesKHR");
	}

	void VulkanSwapchain::CreateImageViews()
	{
		m_ImageViews.clear();
		m_ImageViews.resize(m_Images.size());

		for (uint32_t i = 0; i < m_Images.size(); ++i)
		{
			VkImageViewCreateInfo l_ViewInfo{};
			l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			l_ViewInfo.image = m_Images[i];
			l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			l_ViewInfo.format = m_ImageFormat;
			l_ViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			l_ViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			l_ViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			l_ViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			l_ViewInfo.subresourceRange.baseMipLevel = 0;
			l_ViewInfo.subresourceRange.levelCount = 1;
			l_ViewInfo.subresourceRange.baseArrayLayer = 0;
			l_ViewInfo.subresourceRange.layerCount = 1;

			Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_Allocator, &m_ImageViews[i]), "Failed vkCreateImageView (swapchain)");
		}
	}

	void VulkanSwapchain::DestroyImageViews(const std::vector<VkImageView>& imageViews)
	{
		for (VkImageView it_View : imageViews)
		{
			if (it_View != VK_NULL_HANDLE)
			{
				vkDestroyImageView(m_Device, it_View, m_Allocator);
			}
		}
	}

	void VulkanSwapchain::DestroySwapchain(VkSwapchainKHR swapchainHandle)
	{
		if (swapchainHandle != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_Device, swapchainHandle, m_Allocator);
		}
	}

	VkSurfaceFormatKHR VulkanSwapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
	{
		for (const VkSurfaceFormatKHR& it_Format : formats)
		{
			if (it_Format.format == VK_FORMAT_B8G8R8A8_SRGB && it_Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return it_Format;
			}
		}

		return formats[0];
	}

	VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
	{
		if (m_VSync)
		{
			return VK_PRESENT_MODE_FIFO_KHR;
		}

		for (VkPresentModeKHR it_Mode : presentModes)
		{
			if (it_Mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return it_Mode;
			}
		}

		for (VkPresentModeKHR it_Mode : presentModes)
		{
			if (it_Mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				return it_Mode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t preferredWidth, uint32_t preferredHeight) const
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}

		VkExtent2D l_ActualExtent{};
		l_ActualExtent.width = std::clamp(preferredWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		l_ActualExtent.height = std::clamp(preferredHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return l_ActualExtent;
	}

	VkCompositeAlphaFlagBitsKHR VulkanSwapchain::ChooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) const
	{
		const VkCompositeAlphaFlagBitsKHR l_Options[] =
		{
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
		};

		for (VkCompositeAlphaFlagBitsKHR it_Option : l_Options)
		{
			if (capabilities.supportedCompositeAlpha & it_Option)
			{
				return it_Option;
			}
		}

		return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
}