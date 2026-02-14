#pragma once

#include <vulkan/vulkan.h>

namespace Trinity
{
    class Window;
    class Event;

    class ImGuiLayer
    {
    public:
        ImGuiLayer() = default;
        ~ImGuiLayer() = default;

        void Initialize(Window& window);
        void Shutdown();

        void BeginFrame();
        void EndFrame();
        void OnSwapchainRecreated(uint32_t imageCount, VkFormat colorFormat);

        void OnEvent(Event& event);

    private:
        void InitializeVulkanBackend();
        void ShutdownVulkanBackend();
        void UploadFonts();

    private:
        Window* m_Window = nullptr;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        bool m_Initialized = false;
        bool m_VulkanBackendInitialized = false;
        uint32_t m_SwapchainImageCount = 0;
        VkFormat m_SwapchainColorFormat = VK_FORMAT_UNDEFINED;
    };
}