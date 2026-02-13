#include "Trinity/ImGui/ImGuiLayer.h"

#include "Trinity/Platform/Window.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <Windows.h>

#include <imgui.h>
#include <vulkan/vulkan_win32.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_vulkan.h>

#include <array>
#include <cstdlib>

namespace Trinity
{
    void ImGuiLayer::Initialize(Window& window)
    {
        if (m_Initialized)
        {
            return;
        }

        m_Window = &window;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

        platform_io.Platform_CreateVkSurface = [](ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface) -> int
        {
            VkInstance instance = (VkInstance)vk_instance;
            const VkAllocationCallbacks* allocator = (const VkAllocationCallbacks*)vk_allocator;

            HWND hwnd = (HWND)viewport->PlatformHandleRaw;

            VkWin32SurfaceCreateInfoKHR create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            create_info.hinstance = GetModuleHandle(nullptr);
            create_info.hwnd = hwnd;

            VkSurfaceKHR surface;
            if (vkCreateWin32SurfaceKHR(instance, &create_info, allocator, &surface) != VK_SUCCESS)
            {
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            *out_vk_surface = (ImU64)surface;
            
            return VK_SUCCESS;
        };

        ImGuiIO& l_IO = ImGui::GetIO();
        l_IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        l_IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        l_IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle& l_Style = ImGui::GetStyle();
        if ((l_IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
        {
            l_Style.WindowRounding = 0.0f;
            l_Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        const NativeWindowHandle l_NativeHandle = m_Window->GetNativeHandle();
        if (l_NativeHandle.WindowType != NativeWindowHandle::Type::Win32)
        {
            TR_CORE_CRITICAL("ImGuiLayer currently requires a Win32 window handle");

            std::abort();
        }

        ImGui_ImplWin32_Init(l_NativeHandle.Handle1);
        InitializeVulkanBackend();

        m_Initialized = true;
    }

    void ImGuiLayer::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        ShutdownVulkanBackend();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        m_Window = nullptr;
        m_Initialized = false;
    }

    void ImGuiLayer::BeginFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::EndFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui::Render();
        RenderCommand::RenderImGui(*this);

        ImGuiIO& l_IO = ImGui::GetIO();
        if ((l_IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImGuiLayer::OnEvent(Event& event)
    {
        ImGuiIO& l_IO = ImGui::GetIO();

        const bool l_IsInputEvent = event.IsInCategory(EventCategoryKeyboard) || event.IsInCategory(EventCategoryMouse);
        if (!l_IsInputEvent)
        {
            return;
        }

        const bool l_WantsInputCapture = l_IO.WantCaptureMouse || l_IO.WantCaptureKeyboard;
        if (l_WantsInputCapture)
        {
            event.Handled = true;
        }
    }

    void ImGuiLayer::InitializeVulkanBackend()
    {
        Renderer& l_Renderer = RenderCommand::GetRenderer();
        if (l_Renderer.GetAPI() != RendererAPI::VULKAN)
        {
            return;
        }

        auto* l_VulkanRenderer = dynamic_cast<VulkanRenderer*>(&l_Renderer);
        if (l_VulkanRenderer == nullptr)
        {
            TR_CORE_CRITICAL("Failed to access VulkanRenderer for ImGui initialization");

            std::abort();
        }

        constexpr std::array<VkDescriptorPoolSize, 11> l_PoolSizes =
        {
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo l_PoolCreateInfo{};
        l_PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        l_PoolCreateInfo.maxSets = 1000 * static_cast<uint32_t>(l_PoolSizes.size());
        l_PoolCreateInfo.poolSizeCount = static_cast<uint32_t>(l_PoolSizes.size());
        l_PoolCreateInfo.pPoolSizes = l_PoolSizes.data();

        Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(l_VulkanRenderer->GetVulkanDevice(), &l_PoolCreateInfo, l_VulkanRenderer->GetVulkanAllocator(), &m_DescriptorPool),
            "Failed vkCreateDescriptorPool");

        ImGui_ImplVulkan_InitInfo l_InitInfo{};
        l_InitInfo.ApiVersion = VK_API_VERSION_1_3;
        l_InitInfo.Instance = l_VulkanRenderer->GetVulkanInstance();
        l_InitInfo.PhysicalDevice = l_VulkanRenderer->GetVulkanPhysicalDevice();
        l_InitInfo.Device = l_VulkanRenderer->GetVulkanDevice();
        l_InitInfo.QueueFamily = l_VulkanRenderer->GetVulkanGraphicsQueueFamilyIndex();
        l_InitInfo.Queue = l_VulkanRenderer->GetVulkanGraphicsQueue();
        l_InitInfo.DescriptorPool = m_DescriptorPool;
        l_InitInfo.MinImageCount = l_VulkanRenderer->GetVulkanSwapchainImageCount();
        l_InitInfo.ImageCount = l_VulkanRenderer->GetVulkanSwapchainImageCount();
        l_InitInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        l_InitInfo.UseDynamicRendering = true;
        l_InitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_InitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;

        const VkFormat l_ColorFormat = l_VulkanRenderer->GetVulkanSwapchainImageFormat();
        l_InitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &l_ColorFormat;

        ImGui_ImplVulkan_Init(&l_InitInfo);

        UploadFonts();

        m_VulkanBackendInitialized = true;
    }

    void ImGuiLayer::ShutdownVulkanBackend()
    {
        if (m_VulkanBackendInitialized)
        {
            ImGui_ImplVulkan_Shutdown();
            m_VulkanBackendInitialized = false;
        }

        Renderer& l_Renderer = RenderCommand::GetRenderer();
        if (m_DescriptorPool != VK_NULL_HANDLE && l_Renderer.GetAPI() == RendererAPI::VULKAN)
        {
            auto* l_VulkanRenderer = dynamic_cast<VulkanRenderer*>(&l_Renderer);
            if (l_VulkanRenderer != nullptr)
            {
                vkDestroyDescriptorPool(l_VulkanRenderer->GetVulkanDevice(), m_DescriptorPool, l_VulkanRenderer->GetVulkanAllocator());
            }
        }

        m_DescriptorPool = VK_NULL_HANDLE;
    }

    void ImGuiLayer::UploadFonts()
    {
        Renderer& l_Renderer = RenderCommand::GetRenderer();
        auto* l_VulkanRenderer = dynamic_cast<VulkanRenderer*>(&l_Renderer);
        if (l_VulkanRenderer == nullptr)
        {
            return;
        }

        VkCommandPool l_CommandPool = l_VulkanRenderer->GetVulkanCommandPool(0);
        VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;

        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandPool = l_CommandPool;
        l_AllocInfo.commandBufferCount = 1;

        Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(l_VulkanRenderer->GetVulkanDevice(), &l_AllocInfo, &l_CommandBuffer), "Failed vkAllocateCommandBuffers");

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo), "Failed vkBeginCommandBuffer");
        Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(l_CommandBuffer), "Failed vkEndCommandBuffer");

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;

        Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(l_VulkanRenderer->GetVulkanGraphicsQueue(), 1, &l_SubmitInfo, VK_NULL_HANDLE), "Failed vkQueueSubmit");
        Utilities::VulkanUtilities::VKCheck(vkQueueWaitIdle(l_VulkanRenderer->GetVulkanGraphicsQueue()), "");

        vkFreeCommandBuffers(l_VulkanRenderer->GetVulkanDevice(), l_CommandPool, 1, &l_CommandBuffer);
    }
}