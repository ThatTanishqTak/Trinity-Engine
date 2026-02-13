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

        void OnEvent(Event& event);

    private:
        static void CheckVkResult(VkResult result);
        void InitializeVulkanBackend();
        void ShutdownVulkanBackend();
        void UploadFonts();

    private:
        Window* m_Window = nullptr;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        bool m_Initialized = false;
        bool m_VulkanBackendInitialized = false;
    };
}