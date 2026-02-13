#include "Trinity/ImGui/ImGuiLayer.h"

#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#ifdef _WIN32
#include <Windows.h>
#include <backends/imgui_impl_win32.h>
#endif

#include <backends/imgui_impl_vulkan.h>

#include <cstdlib>

namespace Trinity
{
    void ImGuiLayer::Initialize()
    {
        Initialize(Application::Get().GetWindow());
    }

    void ImGuiLayer::Initialize(Window& window)
    {
        if (m_ContextInitialized)
        {
            TR_CORE_WARN("ImGuiLayer::Initialize called more than once. Ignoring.");

            return;
        }

        m_Window = &window;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

#ifdef _WIN32
        const NativeWindowHandle native = window.GetNativeHandle();
        if (native.WindowType != NativeWindowHandle::Type::Win32 || native.Handle1 == nullptr)
        {
            TR_CORE_CRITICAL("ImGuiLayer requires a Win32 native handle on Windows.");

            std::abort();
        }

        ImGui_ImplWin32_EnableDpiAwareness();

        HWND hwnd = reinterpret_cast<HWND>(native.Handle1);
        if (!ImGui_ImplWin32_Init(hwnd))
        {
            TR_CORE_CRITICAL("ImGui_ImplWin32_Init failed.");

            std::abort();
        }

        m_PlatformInitialized = true;
#else
        (void)window;
        TR_CORE_CRITICAL("ImGuiLayer platform init is only implemented for Win32 in this build.");

        std::abort();
#endif

        m_ContextInitialized = true;
        TR_CORE_INFO("ImGuiLayer initialized (Context + Platform backend). Vulkan backend not initialized yet.");
    }

    void ImGuiLayer::InitializeVulkan(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue,
        VkFormat swapchainColorFormat, uint32_t swapchainImageCount, uint32_t minImageCount)
    {
        if (!m_ContextInitialized)
        {
            TR_CORE_CRITICAL("ImGuiLayer::InitializeVulkan called before Initialize().");

            std::abort();
        }

        if (m_VulkanInitialized)
        {
            TR_CORE_WARN("ImGuiLayer::InitializeVulkan called more than once. Ignoring.");

            return;
        }

        if (instance == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("ImGuiLayer::InitializeVulkan received null Vulkan handles.");

            std::abort();
        }

        if (graphicsQueue == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("ImGuiLayer::InitializeVulkan received null graphics queue.");

            std::abort();
        }

        if (swapchainImageCount == 0)
        {
            TR_CORE_CRITICAL("ImGuiLayer::InitializeVulkan received swapchainImageCount = 0.");

            std::abort();
        }

        m_VkDevice = device;
        m_VkGraphicsQueue = graphicsQueue;
        m_VkGraphicsQueueFamily = graphicsQueueFamily;
        m_SwapchainColorFormat = swapchainColorFormat;
        m_ImageCount = swapchainImageCount;
        m_MinImageCount = (minImageCount == 0) ? 2u : minImageCount;

        CreateVulkanDescriptorPool();

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = physicalDevice;
        initInfo.Device = device;
        initInfo.QueueFamily = graphicsQueueFamily;
        initInfo.Queue = graphicsQueue;
        initInfo.DescriptorPool = m_DescriptorPool;
        initInfo.MinImageCount = (int)m_MinImageCount;
        initInfo.ImageCount = (int)m_ImageCount;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {};
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainColorFormat;

        // No render pass needed for dynamic rendering.
        if (!ImGui_ImplVulkan_Init(&initInfo))
        {
            TR_CORE_CRITICAL("ImGui_ImplVulkan_Init failed.");

            std::abort();
        }

        UploadFonts();

        m_VulkanInitialized = true;
        TR_CORE_INFO("ImGuiLayer initialized (Vulkan backend). ImageCount = {}, MinImageCount = {}", m_ImageCount, m_MinImageCount);
    }

    void ImGuiLayer::Shutdown()
    {
        if (!m_ContextInitialized)
        {
            return;
        }

        if (m_VulkanInitialized)
        {
            vkDeviceWaitIdle(m_VkDevice);
            ImGui_ImplVulkan_Shutdown();
            DestroyVulkanDescriptorPool();

            m_VulkanInitialized = false;
            m_VkDevice = VK_NULL_HANDLE;
            m_VkGraphicsQueue = VK_NULL_HANDLE;
            m_VkGraphicsQueueFamily = 0;
        }

#ifdef _WIN32
        if (m_PlatformInitialized)
        {
            ImGui_ImplWin32_Shutdown();
            m_PlatformInitialized = false;
        }
#endif

        ImGui::DestroyContext();
        m_ContextInitialized = false;
        m_Window = nullptr;

        TR_CORE_INFO("ImGuiLayer shutdown complete.");
    }

    void ImGuiLayer::BeginFrame()
    {
        if (!m_ContextInitialized)
        {
            return;
        }

#ifdef _WIN32
        if (m_PlatformInitialized)
        {
            ImGui_ImplWin32_NewFrame();
        }
#endif

        if (m_VulkanInitialized)
        {
            ImGui_ImplVulkan_NewFrame();
        }

        ImGui::NewFrame();
    }

    void ImGuiLayer::EndFrame(VkCommandBuffer commandBuffer)
    {
        if (!m_ContextInitialized)
        {
            return;
        }

        ImGui::Render();

        if (!m_VulkanInitialized)
        {
            return;
        }

        if (commandBuffer == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("ImGuiLayer::EndFrame called with VK_NULL_HANDLE command buffer.");
            std::abort();
        }

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    void ImGuiLayer::OnEvent(Event& e)
    {
        if (!m_ContextInitialized)
        {
            return;
        }

        const ImGuiIO& io = ImGui::GetIO();

        if (e.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse)
        {
            e.Handled = true;
        }

        if (e.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard)
        {
            e.Handled = true;
        }
    }

    void ImGuiLayer::OnSwapchainRecreated(uint32_t minImageCount, uint32_t imageCount)
    {
        if (!m_VulkanInitialized)
        {
            return;
        }

        m_MinImageCount = (minImageCount == 0) ? 2u : minImageCount;
        m_ImageCount = (imageCount == 0) ? 2u : imageCount;

        ImGui_ImplVulkan_SetMinImageCount((int)m_MinImageCount);
    }

    void ImGuiLayer::CreateVulkanDescriptorPool()
    {
        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            return;
        }

        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * (uint32_t)(sizeof(poolSizes) / sizeof(poolSizes[0]));
        poolInfo.poolSizeCount = (uint32_t)(sizeof(poolSizes) / sizeof(poolSizes[0]));
        poolInfo.pPoolSizes = poolSizes;

        Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_VkDevice, &poolInfo, nullptr, &m_DescriptorPool), "vkCreateDescriptorPool");
    }

    void ImGuiLayer::DestroyVulkanDescriptorPool()
    {
        if (m_DescriptorPool == VK_NULL_HANDLE || m_VkDevice == VK_NULL_HANDLE)
        {
            return;
        }

        vkDestroyDescriptorPool(m_VkDevice, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    void ImGuiLayer::UploadFonts()
    {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_VkGraphicsQueueFamily;

        Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_VkDevice, &poolInfo, nullptr, &commandPool), "vkCreateCommandPool (ImGui fonts)");

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_VkDevice, &allocInfo, &commandBuffer), "vkAllocateCommandBuffers (ImGui fonts)");

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer (ImGui fonts)");
        Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer (ImGui fonts)");

        VkFence fence = VK_NULL_HANDLE;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_VkDevice, &fenceInfo, nullptr, &fence), "vkCreateFence (ImGui fonts)");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_VkGraphicsQueue, 1, &submitInfo, fence), "vkQueueSubmit (ImGui fonts)");
        Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_VkDevice, 1, &fence, VK_TRUE, UINT64_MAX), "vkWaitForFences (ImGui fonts)");

        vkDestroyFence(m_VkDevice, fence, nullptr);

        vkFreeCommandBuffers(m_VkDevice, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(m_VkDevice, commandPool, nullptr);
    }
}
