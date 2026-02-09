#pragma once

#include <vulkan/vulkan.h>

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <vector>

namespace Trinity
{
	class VulkanContext
	{
	public:
		void Initialize(HWND windowHandle, HINSTANCE windowInstance);
		void Shutdown();

		VkInstance GetInstance() const { return m_Instance; }
		VkAllocationCallbacks* GetAllocator() const { return m_Allocator; }
		VkSurfaceKHR GetSurface() const { return m_Surface; }

	private:
		void CreateInstance();
		void CreateSurface(HWND windowHandle, HINSTANCE windowInstance);

		void DestroySurface();
		void DestroyInstance();

		std::vector<const char*> GetRequiredExtensions() const;
		std::vector<const char*> GetRequiredLayers() const;

		void SetupDebugMessenger();
		void DestroyDebugMessenger();

		bool IsInstanceExtensionSupported(const char* extensionName) const;
		bool IsInstanceLayerSupported(const char* layerName) const;

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		HWND m_WindowHandle = nullptr;
		HINSTANCE m_WindowInstance = nullptr;

#ifdef _DEBUG
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
#endif
		std::vector<const char*> m_RequiredExtensions{};
		std::vector<const char*> m_RequiredLayers{};
	};
}