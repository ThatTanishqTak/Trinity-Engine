#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Utilities.h"

#include <set>
#include <cstring>

namespace Trinity
{
	static const char* s_DefaultDeviceExtensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	void VulkanDevice::Initialize(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks* allocator)
	{
		TR_CORE_TRACE("Initializing VulkanDevice");

		if (instance == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanDevice::Initialize called with VK_NULL_HANDLE instance");

			return;
		}
		if (surface == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanDevice::Initialize called with VK_NULL_HANDLE surface");

			return;
		}

		m_Instance = instance;
		m_Surface = surface;
		m_Allocator = allocator;

		m_DeviceExtensions.clear();
		for (const char* it_Ext : s_DefaultDeviceExtensions)
		{
			m_DeviceExtensions.push_back(it_Ext);
		}

#if defined(__APPLE__)
		// MoltenVK sometimes requires this for swapchain to work.
		m_DeviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

		PickPhysicalDevice();
		CreateLogicalDevice();

		TR_CORE_TRACE("VulkanDevice initialized");
	}

	void VulkanDevice::Shutdown()
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_TRACE("Destroying Vulkan logical device");
			vkDeviceWaitIdle(m_Device);
			vkDestroyDevice(m_Device, m_Allocator);
			m_Device = VK_NULL_HANDLE;
		}

		m_PhysicalDevice = VK_NULL_HANDLE;
		m_GraphicsQueue = VK_NULL_HANDLE;
		m_PresentQueue = VK_NULL_HANDLE;
		m_TransferQueue = VK_NULL_HANDLE;
		m_ComputeQueue = VK_NULL_HANDLE;
		m_QueueFamilyIndices = {};
		m_Properties = {};
		m_Features = {};
		m_MemoryProperties = {};
		m_DeviceExtensions.clear();

		m_Instance = VK_NULL_HANDLE;
		m_Surface = VK_NULL_HANDLE;
		m_Allocator = nullptr;
	}

	uint32_t VulkanDevice::GetGraphicsQueueFamilyIndex() const
	{
		return m_QueueFamilyIndices.GraphicsFamily.value_or(UINT32_MAX);
	}

	uint32_t VulkanDevice::GetPresentQueueFamilyIndex() const
	{
		return m_QueueFamilyIndices.PresentFamily.value_or(UINT32_MAX);
	}

	uint32_t VulkanDevice::GetTransferQueueFamilyIndex() const
	{
		return m_QueueFamilyIndices.TransferFamily.value_or(UINT32_MAX);
	}

	uint32_t VulkanDevice::GetComputeQueueFamilyIndex() const
	{
		return m_QueueFamilyIndices.ComputeFamily.value_or(UINT32_MAX);
	}

	uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; ++i)
		{
			const bool l_TypeMatches = (typeFilter & (1u << i)) != 0;
			const bool l_FlagsMatch = (m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

			if (l_TypeMatches && l_FlagsMatch)
			{
				return i;
			}
		}

		TR_CORE_CRITICAL("Failed to find suitable memory type (typeFilter: {}, properties: {})", typeFilter, properties);

		return UINT32_MAX;
	}

	VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
	{
		for (VkFormat it_Format : candidates)
		{
			VkFormatProperties l_Props{};
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, it_Format, &l_Props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (l_Props.linearTilingFeatures & features) == features)
			{
				return it_Format;
			}

			if (tiling == VK_IMAGE_TILING_OPTIMAL && (l_Props.optimalTilingFeatures & features) == features)
			{
				return it_Format;
			}
		}

		TR_CORE_CRITICAL("Failed to find supported format");

		return VK_FORMAT_UNDEFINED;
	}

	VkFormat VulkanDevice::FindDepthFormat() const
	{
		return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	VulkanDevice::SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkPhysicalDevice physicalDevice) const
	{
		SwapchainSupportDetails l_Details{};

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &l_Details.Capabilities);

		uint32_t l_FormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &l_FormatCount, nullptr);
		if (l_FormatCount != 0)
		{
			l_Details.Formats.resize(l_FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &l_FormatCount, l_Details.Formats.data());
		}

		uint32_t l_PresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &l_PresentModeCount, nullptr);
		if (l_PresentModeCount != 0)
		{
			l_Details.PresentModes.resize(l_PresentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &l_PresentModeCount, l_Details.PresentModes.data());
		}

		return l_Details;
	}

	void VulkanDevice::PickPhysicalDevice()
	{
		TR_CORE_TRACE("Selecting Vulkan physical device");

		uint32_t l_DeviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &l_DeviceCount, nullptr);
		if (l_DeviceCount == 0)
		{
			TR_CORE_CRITICAL("No Vulkan physical devices found");

			return;
		}

		std::vector<VkPhysicalDevice> l_Devices(l_DeviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &l_DeviceCount, l_Devices.data());

		VkPhysicalDevice l_BestDevice = VK_NULL_HANDLE;
		int l_BestScore = -1;

		for (VkPhysicalDevice it_Device : l_Devices)
		{
			if (!IsDeviceSuitable(it_Device))
			{
				continue;
			}

			const int l_Score = ScorePhysicalDevice(it_Device);
			if (l_Score > l_BestScore)
			{
				l_BestScore = l_Score;
				l_BestDevice = it_Device;
			}
		}

		if (l_BestDevice == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("Failed to find a suitable Vulkan physical device");

			return;
		}

		m_PhysicalDevice = l_BestDevice;
		m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_Features);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

		TR_CORE_TRACE("Selected GPU: {}", m_Properties.deviceName);
	}

	void VulkanDevice::CreateLogicalDevice()
	{
		TR_CORE_TRACE("Creating Vulkan logical device");

		m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);
		if (!m_QueueFamilyIndices.IsComplete())
		{
			TR_CORE_CRITICAL("Queue family selection incomplete (graphics/present missing)");

			return;
		}

		std::set<uint32_t> l_UniqueQueueFamilies;
		l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.GraphicsFamily.value());
		l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.PresentFamily.value());
		if (m_QueueFamilyIndices.TransferFamily.has_value())
		{
			l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.TransferFamily.value());
		}

		if (m_QueueFamilyIndices.ComputeFamily.has_value())
		{
			l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.ComputeFamily.value());
		}

		float l_QueuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> l_QueueCreateInfos;
		l_QueueCreateInfos.reserve(l_UniqueQueueFamilies.size());

		for (uint32_t it_Family : l_UniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo l_QueueCreateInfo{};
			l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			l_QueueCreateInfo.queueFamilyIndex = it_Family;
			l_QueueCreateInfo.queueCount = 1;
			l_QueueCreateInfo.pQueuePriorities = &l_QueuePriority;
			l_QueueCreateInfos.push_back(l_QueueCreateInfo);
		}

		VkPhysicalDeviceFeatures l_EnabledFeatures{};
		// Start small: enable only what you actually use.
		if (m_Features.samplerAnisotropy)
		{
			l_EnabledFeatures.samplerAnisotropy = VK_TRUE;
		}

		VkDeviceCreateInfo l_CreateInfo{};
		l_CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		l_CreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueCreateInfos.size());
		l_CreateInfo.pQueueCreateInfos = l_QueueCreateInfos.data();
		l_CreateInfo.pEnabledFeatures = &l_EnabledFeatures;
		l_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		l_CreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.empty() ? nullptr : m_DeviceExtensions.data();
		l_CreateInfo.enabledLayerCount = 0;
		l_CreateInfo.ppEnabledLayerNames = nullptr;

		Utilities::VulkanUtilities::VKCheck(vkCreateDevice(m_PhysicalDevice, &l_CreateInfo, m_Allocator, &m_Device), "Failed vkCreateDevice");

		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);

		if (m_QueueFamilyIndices.TransferFamily.has_value())
		{
			vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.TransferFamily.value(), 0, &m_TransferQueue);
		}
		else
		{
			m_TransferQueue = m_GraphicsQueue;
		}

		if (m_QueueFamilyIndices.ComputeFamily.has_value())
		{
			vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.ComputeFamily.value(), 0, &m_ComputeQueue);
		}
		else
		{
			m_ComputeQueue = m_GraphicsQueue;
		}

		TR_CORE_TRACE("Logical device created");
	}

	bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice physicalDevice) const
	{
		const QueueFamilyIndices l_Indices = FindQueueFamilies(physicalDevice);
		if (!l_Indices.IsComplete())
		{
			return false;
		}

		if (!CheckDeviceExtensionSupport(physicalDevice))
		{
			return false;
		}

		const SwapchainSupportDetails l_SwapchainSupport = QuerySwapchainSupport(physicalDevice);
		const bool l_SwapchainAdequate = !l_SwapchainSupport.Formats.empty() && !l_SwapchainSupport.PresentModes.empty();
		if (!l_SwapchainAdequate)
		{
			return false;
		}

		return true;
	}

	bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) const
	{
		uint32_t l_ExtCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &l_ExtCount, nullptr);
		std::vector<VkExtensionProperties> l_Available(l_ExtCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &l_ExtCount, l_Available.data());

		for (const char* it_Required : m_DeviceExtensions)
		{
			bool l_Found = false;
			for (const auto& it_Av : l_Available)
			{
				if (std::strcmp(it_Required, it_Av.extensionName) == 0)
				{
					l_Found = true;
					break;
				}
			}

			if (!l_Found)
			{
				return false;
			}
		}

		return true;
	}

	int VulkanDevice::ScorePhysicalDevice(VkPhysicalDevice physicalDevice) const
	{
		VkPhysicalDeviceProperties l_Props{};
		vkGetPhysicalDeviceProperties(physicalDevice, &l_Props);

		int l_Score = 0;
		if (l_Props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			l_Score += 1000;
		}

		// Bigger limits are generally better.
		l_Score += static_cast<int>(l_Props.limits.maxImageDimension2D);

		return l_Score;
	}

	VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice physicalDevice) const
	{
		QueueFamilyIndices l_Indices;

		uint32_t l_QueueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &l_QueueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> l_QueueFamilies(l_QueueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &l_QueueFamilyCount, l_QueueFamilies.data());

		std::optional<uint32_t> l_DedicatedTransfer;
		std::optional<uint32_t> l_DedicatedCompute;

		for (uint32_t i = 0; i < l_QueueFamilyCount; ++i)
		{
			const VkQueueFamilyProperties& it_Family = l_QueueFamilies[i];
			if (it_Family.queueCount == 0)
			{
				continue;
			}

			if ((it_Family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && !l_Indices.GraphicsFamily.has_value())
			{
				l_Indices.GraphicsFamily = i;
			}

			VkBool32 l_PresentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_Surface, &l_PresentSupport);
			if (l_PresentSupport && !l_Indices.PresentFamily.has_value())
			{
				l_Indices.PresentFamily = i;
			}

			const bool l_HasTransfer = (it_Family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
			const bool l_HasCompute = (it_Family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
			const bool l_HasGraphics = (it_Family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;

			if (l_HasTransfer)
			{
				if (!l_HasGraphics)
				{
					l_DedicatedTransfer = i;
				}
				else if (!l_Indices.TransferFamily.has_value())
				{
					l_Indices.TransferFamily = i;
				}
			}

			if (l_HasCompute)
			{
				if (!l_HasGraphics)
				{
					l_DedicatedCompute = i;
				}
				else if (!l_Indices.ComputeFamily.has_value())
				{
					l_Indices.ComputeFamily = i;
				}
			}

			// Note: we intentionally don't early-break here so we can still prefer dedicated transfer/compute queues.
		}

		if (l_DedicatedTransfer.has_value())
		{
			l_Indices.TransferFamily = l_DedicatedTransfer;
		}
		
		if (l_DedicatedCompute.has_value())
		{
			l_Indices.ComputeFamily = l_DedicatedCompute;
		}

		if (!l_Indices.TransferFamily.has_value() && l_Indices.GraphicsFamily.has_value())
		{
			l_Indices.TransferFamily = l_Indices.GraphicsFamily;
		}

		if (!l_Indices.ComputeFamily.has_value() && l_Indices.GraphicsFamily.has_value())
		{
			l_Indices.ComputeFamily = l_Indices.GraphicsFamily;
		}

		return l_Indices;
	}
}