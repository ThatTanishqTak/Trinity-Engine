#pragma once

#include <vulkan/vulkan.h>

namespace Trinity
{
	class Window;

	class VulkanInstance
	{
	public:
		VulkanInstance() = default;
		~VulkanInstance() = default;

		void Initialize(Window& window, bool enableValidation);
		void Shutdown();

		VkInstance GetInstance() const { return m_Instance; }
		VkSurfaceKHR GetSurface() const { return m_Surface; }
		bool IsValidationEnabled() const { return m_ValidationEnabled; }

	private:
		void CreateInstance(bool enableValidation);
		void CreateSurface(Window& window);

		bool CheckValidationLayerSupport() const;

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		bool m_ValidationEnabled = false;
	};
}