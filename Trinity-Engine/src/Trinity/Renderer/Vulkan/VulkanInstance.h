#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <cstdint>

namespace Trinity
{
	class VulkanInstance
	{
	public:
		VulkanInstance() = default;
		~VulkanInstance() = default;

		VulkanInstance(const VulkanInstance&) = delete;
		VulkanInstance& operator=(const VulkanInstance&) = delete;
		VulkanInstance(VulkanInstance&&) = delete;
		VulkanInstance& operator=(VulkanInstance&&) = delete;

		VkInstance GetInstance() const { return m_InstanceHandle; }
		VkAllocationCallbacks* GetAllocator() { return m_Allocator; }

		const std::vector<const char*>& GetEnabledLayers() const { return m_EnabledLayers; }
		const std::vector<const char*>& GetEnabledExtensions() const { return m_EnabledExtensions; }

		void Initialize();
		void Shutdown();

	private:
		void CreateInstance();
		void SetupDebugMessenger();

		void DestroyInstance();
		void DestroyDebugMessenger();

		bool CheckValidationSupport() const;
		bool CheckInstanceExtensionSupport(const char* extensionName) const;

		void CollectRequiredExtensions();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo) const;

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pMessenger);

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator);

	private:
		VkInstance m_InstanceHandle = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

#ifdef _DEBUG
		bool m_EnableValidationLayers = true;
#else
		bool m_EnableValidationLayers = false;
#endif

		std::vector<const char*> m_EnabledLayers;
		std::vector<const char*> m_EnabledExtensions;

		uint32_t m_LayerCount = 0;
		uint32_t m_ExtensionCount = 0;

		// TODO: Custom allocator
		VkAllocationCallbacks* m_Allocator = nullptr;
	};
}