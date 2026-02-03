#include "Trinity/Renderer/Vulkan/VulkanInstance.h"

#include "Trinity/Utilities/Utilities.h"

#include <cstring>
#include <cstdlib>

namespace Trinity
{
	static const char* s_ValidationLayers[] =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	void VulkanInstance::Initialize()
	{
		CreateInstance();
		SetupDebugMessenger();
	}

	void VulkanInstance::Shutdown()
	{
		DestroyDebugMessenger();
		DestroyInstance();
	}

	void VulkanInstance::CreateInstance()
	{
		TR_CORE_TRACE("Creating vulkan instace");

		// If validation is requested but unavailable, disable it
		if (m_EnableValidationLayers && !CheckValidationSupport())
		{
			TR_CORE_WARN("Validation layers requested but not available. Disabling validation layers.");
			m_EnableValidationLayers = false;
		}

		// Build extension list BEFORE vkCreateInstance
		CollectRequiredExtensions();

		m_EnabledLayers.clear();
		if (m_EnableValidationLayers)
		{
			m_EnabledLayers.push_back(s_ValidationLayers[0]);
		}

		m_ExtensionCount = static_cast<uint32_t>(m_EnabledExtensions.size());
		m_LayerCount = static_cast<uint32_t>(m_EnabledLayers.size());

		TR_CORE_TRACE("Enabled instance extensions:");
		for (const char* it_Extension : m_EnabledExtensions)
		{
			TR_CORE_TRACE("  {}", it_Extension);
		}

		VkApplicationInfo l_ApplicationInfo{};
		l_ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		l_ApplicationInfo.apiVersion = VK_API_VERSION_1_3;
		l_ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		l_ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		l_ApplicationInfo.pApplicationName = "Trinity Application";
		l_ApplicationInfo.pEngineName = "Trinity Engine";

		VkInstanceCreateInfo l_InstanceInfo{};
		l_InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		l_InstanceInfo.pApplicationInfo = &l_ApplicationInfo;
		l_InstanceInfo.enabledExtensionCount = m_ExtensionCount;
		l_InstanceInfo.ppEnabledExtensionNames = m_ExtensionCount > 0 ? m_EnabledExtensions.data() : nullptr;
		l_InstanceInfo.enabledLayerCount = m_LayerCount;
		l_InstanceInfo.ppEnabledLayerNames = m_LayerCount > 0 ? m_EnabledLayers.data() : nullptr;

#ifdef __APPLE__
		l_InstanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

		// Only legal to chain VkDebugUtilsMessengerCreateInfoEXT if VK_EXT_debug_utils is enabled.
		VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
		if (m_EnableValidationLayers && CheckInstanceExtensionSupport(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		{
			PopulateDebugMessengerCreateInfo(l_DebugCreateInfo);
			l_InstanceInfo.pNext = &l_DebugCreateInfo;
		}
		else
		{
			l_InstanceInfo.pNext = nullptr;
		}

		Utilities::VulkanUtilities::VKCheck(vkCreateInstance(&l_InstanceInfo, m_Allocator, &m_InstanceHandle), "Failed vkCreateInstance");

		TR_CORE_TRACE("Vulkan instace created");
	}

	void VulkanInstance::SetupDebugMessenger()
	{
		TR_CORE_TRACE("Setting up debug messenger");

		if (!m_EnableValidationLayers || m_InstanceHandle == VK_NULL_HANDLE)
		{
			return;
		}

		if (m_DebugMessenger != VK_NULL_HANDLE)
		{
			return;
		}

		if (!CheckInstanceExtensionSupport(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		{
			TR_CORE_WARN("VK_EXT_debug_utils not available. Skipping debug messenger");

			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT l_CreateInfo{};
		PopulateDebugMessengerCreateInfo(l_CreateInfo);

		Utilities::VulkanUtilities::VKCheck(CreateDebugUtilsMessengerEXT(m_InstanceHandle, &l_CreateInfo, nullptr, &m_DebugMessenger), "Failed CreateDebugUtilsMessengerEXT");

		TR_CORE_TRACE("Debug messenger setup complete");
	}

	void VulkanInstance::DestroyInstance()
	{
		TR_CORE_TRACE("Destroying vulkan instance");

		if (m_InstanceHandle == VK_NULL_HANDLE)
		{
			return;
		}

		vkDestroyInstance(m_InstanceHandle, m_Allocator);
		m_InstanceHandle = VK_NULL_HANDLE;

		TR_CORE_TRACE("Vulkan instance destroyed");
	}

	void VulkanInstance::DestroyDebugMessenger()
	{
		TR_CORE_TRACE("Destroying debug messenger");

		if (m_DebugMessenger == VK_NULL_HANDLE || m_InstanceHandle == VK_NULL_HANDLE)
		{
			return;
		}

		DestroyDebugUtilsMessengerEXT(m_InstanceHandle, m_DebugMessenger, nullptr);
		m_DebugMessenger = VK_NULL_HANDLE;

		TR_CORE_TRACE("Debug messenger destroyed");
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------//

	bool VulkanInstance::CheckValidationSupport() const
	{
		uint32_t l_LayerCount = 0;
		vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

		std::vector<VkLayerProperties> l_AvailableLayers(l_LayerCount);
		vkEnumerateInstanceLayerProperties(&l_LayerCount, l_AvailableLayers.data());

		for (const char* l_RequestedLayer : s_ValidationLayers)
		{
			bool l_Found = false;

			for (const auto& it_Layer : l_AvailableLayers)
			{
				if (std::strcmp(l_RequestedLayer, it_Layer.layerName) == 0)
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

	bool VulkanInstance::CheckInstanceExtensionSupport(const char* extensionName) const
	{
		uint32_t l_Count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &l_Count, nullptr);

		std::vector<VkExtensionProperties> l_Extensions(l_Count);
		vkEnumerateInstanceExtensionProperties(nullptr, &l_Count, l_Extensions.data());

		for (const auto& it_Extenstion : l_Extensions)
		{
			if (std::strcmp(extensionName, it_Extenstion.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	void VulkanInstance::CollectRequiredExtensions()
	{
		m_EnabledExtensions.clear();

		auto a_AddRequired = [this](const char* extensionName)
		{
			for (const char* it_Extension : m_EnabledExtensions)
			{
				if (std::strcmp(it_Extension, extensionName) == 0)
				{
					return;
				}
			}

			if (!CheckInstanceExtensionSupport(extensionName))
			{
				TR_CORE_CRITICAL("Required Vulkan instance extension '{}' is not supported.", extensionName);

				std::abort();
			}

			m_EnabledExtensions.push_back(extensionName);
		};

		// Surface is required for any presentation
		a_AddRequired(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(_WIN32)
		// Win32 surface
		a_AddRequired("VK_KHR_win32_surface");
#endif

		// Optional debug utils
		if (m_EnableValidationLayers)
		{
			if (CheckInstanceExtensionSupport(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
			{
				m_EnabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			else
			{
				TR_CORE_WARN("VK_EXT_debug_utils not supported. Debug messenger will be disabled.");
			}
		}

#if defined(__APPLE__)
		// MoltenVK portability
		a_AddRequired(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
	}

	void VulkanInstance::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo) const
	{
		messengerCreateInfo = {};
		messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		messengerCreateInfo.pfnUserCallback = DebugCallback;
		messengerCreateInfo.pUserData = nullptr;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
	{
		if (!pCallbackData || !pCallbackData->pMessage)
		{
			return VK_FALSE;
		}

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			TR_CORE_ERROR("[VULKAN] {}", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			TR_CORE_WARN("[VULKAN] {}", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			TR_CORE_TRACE("[VULKAN] {}", pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

	VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pMessenger)
	{
		auto a_Function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

		if (a_Function != nullptr)
		{
			return a_Function(instance, pCreateInfo, pAllocator, pMessenger);
		}

		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
	{
		auto a_Function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

		if (a_Function != nullptr)
		{
			a_Function(instance, messenger, pAllocator);
		}
	}
}