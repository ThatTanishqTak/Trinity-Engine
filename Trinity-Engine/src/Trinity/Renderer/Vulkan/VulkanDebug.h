#pragma once

#include <vulkan/vulkan.h>

namespace Trinity
{
	class VulkanDebug
	{
	public:
		VulkanDebug() = default;
		~VulkanDebug() = default;

		void Initialize(VkInstance instance);
		void Shutdown();

		static void PopulateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	};
}