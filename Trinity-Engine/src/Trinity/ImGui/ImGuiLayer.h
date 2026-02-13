#pragma once

#include <imgui.h>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace Trinity
{
    class Window;
    class Event;

    class ImGuiLayer
    {
    public:
        void Initialize();

        // Creates ImGui context + platform backend.
        void Initialize(Window& window);

        void InitializeVulkan(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue, VkFormat swapchainColorFormat,
            uint32_t swapchainImageCount, uint32_t minImageCount = 2);

        void Shutdown();

        void BeginFrame();
        void EndFrame(VkCommandBuffer commandBuffer);

        void OnEvent(Event& e);

        void OnSwapchainRecreated(uint32_t minImageCount, uint32_t imageCount);

        bool IsInitialized() const { return m_ContextInitialized; }
        bool IsVulkanInitialized() const { return m_VulkanInitialized; }

    private:
        void CreateVulkanDescriptorPool();
        void DestroyVulkanDescriptorPool();
        void UploadFonts();

    private:
        Window* m_Window = nullptr;

        bool m_ContextInitialized = false;
        bool m_PlatformInitialized = false;
        bool m_VulkanInitialized = false;

        VkDevice m_VkDevice = VK_NULL_HANDLE;
        VkQueue m_VkGraphicsQueue = VK_NULL_HANDLE;
        uint32_t m_VkGraphicsQueueFamily = 0;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

        uint32_t m_MinImageCount = 2;
        uint32_t m_ImageCount = 2;
        VkFormat m_SwapchainColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    };
}