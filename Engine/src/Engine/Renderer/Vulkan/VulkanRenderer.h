#pragma once

#include "Engine/Utilities/Utilities.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <cstdint>

struct GLFWwindow;

namespace Engine
{
    class Window;

    class VulkanRenderer
    {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer();

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;
        VulkanRenderer(VulkanRenderer&&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        void Initialize(Window& window);
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        void OnResize(uint32_t width, uint32_t height);

    private:
        void CreateInstance();
        void SetupDebugMessenger();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapchain();
        void CreateImageViews();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void CleanupSwapchain();
        void RecreateSwapchain();

        bool CheckValidationLayerSupport() const;
        std::vector<const char*> GetRequiredExtensions() const;

    private:
        struct QueueFamilyIndices
        {
            std::optional<uint32_t> GraphicsFamily;
            std::optional<uint32_t> PresentFamily;

            bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
        };

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

        struct SwapchainSupportDetails
        {
            VkSurfaceCapabilitiesKHR Capabilities{};
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR> PresentModes;
        };

        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) const;

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
        );

        VkResult CreateDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pMessenger
        );

        void DestroyDebugUtilsMessengerEXT(
            VkInstance instance,
            VkDebugUtilsMessengerEXT messenger,
            const VkAllocationCallbacks* pAllocator
        );

    private:
        Window* m_Window = nullptr;
        GLFWwindow* m_GLFWWindow = nullptr;

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_SwapchainImages;
        VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_SwapchainExtent{};
        std::vector<VkImageView> m_SwapchainImageViews;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        static constexpr int s_MaxFramesInFlight = 2;
        int m_CurrentFrame = 0;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;

        bool m_FramebufferResized = false;
        uint32_t m_ImageIndex = 0;

        bool m_FrameInProgress = false;
        bool m_Initialized = false;

    private:
#ifdef TR_ENABLE_VK_VALIDATION
        const bool m_EnableValidationLayers = true;
#else
        const bool m_EnableValidationLayers = false;
#endif

        const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
}