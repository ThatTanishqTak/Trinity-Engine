#pragma once

#include "Trinity/Platform/Window.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace Trinity
{
	class VulkanContext
	{
	public:
		void Initialize(const NativeWindowHandle& nativeWindowHandle);
		void Shutdown();

		VkInstance GetInstance() const { return m_Instance; }
		VkAllocationCallbacks* GetAllocator() const { return m_Allocator; }
		VkSurfaceKHR GetSurface() const { return m_Surface; }

	private:
		void CreateInstance();
		void CreateSurface(const NativeWindowHandle& nativeWindowHandle);

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


#ifdef _DEBUG
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
#endif
		std::vector<const char*> m_RequiredExtensions{};
		std::vector<const char*> m_RequiredLayers{};
	};
}