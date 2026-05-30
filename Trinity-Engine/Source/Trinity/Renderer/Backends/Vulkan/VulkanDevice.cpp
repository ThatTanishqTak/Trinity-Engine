#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>

#include <set>
#include <vector>

#include <Trinity/Renderer/Backends/Vulkan/VulkanSwapchain.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    VulkanDevice::VulkanDevice(const NativeWindowHandle& window, const std::string& applicationName, bool enableValidation) : m_Window(window), m_ApplicationName(applicationName), m_EnableValidation(enableValidation)
    {

    }

    VulkanDevice::~VulkanDevice()
    {
        Shutdown();
    }

    bool VulkanDevice::Initialize()
    {
        if (m_Initialized)
        {
            return true;
        }

        if (!m_Instance.Initialize(m_ApplicationName, m_EnableValidation))
        {
            return false;
        }

        if (!m_Surface.Initialize(m_Instance.GetHandle(), m_Window))
        {
            return false;
        }

        VulkanFeatures l_Required;
        if (!m_PhysicalDevice.Select(m_Instance.GetHandle(), m_Surface.GetHandle(), l_Required))
        {
            return false;
        }

        if (!CreateLogicalDevice())
        {
            return false;
        }

        if (!m_Allocator.Initialize(m_Instance.GetHandle(), m_PhysicalDevice.GetHandle(), m_Device))
        {
            return false;
        }

        if (!m_Commands.Initialize(m_Device, m_GraphicsQueueFamily, m_GraphicsQueue))
        {
            return false;
        }

        QueryCapabilities();

        m_Initialized = true;
        TR_CORE_INFO("VulkanDevice: initialized");

        return true;
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);

            VmaAllocator l_Allocator = m_Allocator.GetHandle();

            m_Pipelines.ForEachAlive([&](VulkanPipelineResource& resource)
            {
                if (resource.Pipeline != VK_NULL_HANDLE)
                {
                    vkDestroyPipeline(m_Device, resource.Pipeline, nullptr);
                }

                if (resource.Layout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(m_Device, resource.Layout, nullptr);
                }
            });

            m_Shaders.ForEachAlive([&](VulkanShaderResource& resource)
            {
                if (resource.Module != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(m_Device, resource.Module, nullptr);
                }
            });
            m_Samplers.ForEachAlive([&](VulkanSamplerResource& resource)
            {
                if (resource.Sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(m_Device, resource.Sampler, nullptr);
                }
            });

            m_Textures.ForEachAlive([&](VulkanTextureResource& resource)
            {
                if (resource.OwnsView && resource.View != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(m_Device, resource.View, nullptr);
                }

                if (resource.OwnsImage && resource.Image != VK_NULL_HANDLE)
                {
                    vmaDestroyImage(l_Allocator, resource.Image, resource.Allocation);
                }
            });

            m_Buffers.ForEachAlive([&](VulkanBufferResource& resource)
            {
                if (resource.Buffer != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(l_Allocator, resource.Buffer, resource.Allocation);
                }
            });

            m_Commands.Shutdown();
            m_Allocator.Shutdown();

            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        m_Surface.Shutdown();
        m_Instance.Shutdown();

        m_Initialized = false;
    }

    BufferHandle VulkanDevice::CreateBuffer(const BufferDescription&)
    {
        TR_CORE_WARN("VulkanDevice: CreateBuffer not yet implemented");
        return BufferHandle();
    }

    TextureHandle VulkanDevice::CreateTexture(const TextureDescription&)
    {
        TR_CORE_WARN("VulkanDevice: CreateTexture not yet implemented");
        return TextureHandle();
    }

    SamplerHandle VulkanDevice::CreateSampler(const SamplerDescription&)
    {
        TR_CORE_WARN("VulkanDevice: CreateSampler not yet implemented");
        return SamplerHandle();
    }

    ShaderHandle VulkanDevice::CreateShader(const ShaderDescription&)
    {
        TR_CORE_WARN("VulkanDevice: CreateShader not yet implemented");
        return ShaderHandle();
    }

    PipelineHandle VulkanDevice::CreatePipeline(const PipelineDescription&)
    {
        TR_CORE_WARN("VulkanDevice: CreatePipeline not yet implemented");
        return PipelineHandle();
    }

    void VulkanDevice::DestroyBuffer(BufferHandle handle)
    {
        VulkanBufferResource l_Resource{};
        if (m_Buffers.Free(handle, l_Resource) && l_Resource.Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_Allocator.GetHandle(), l_Resource.Buffer, l_Resource.Allocation);
        }
    }

    void VulkanDevice::DestroyTexture(TextureHandle handle)
    {
        VulkanTextureResource l_Resource{};
        if (m_Textures.Free(handle, l_Resource))
        {
            if (l_Resource.OwnsView && l_Resource.View != VK_NULL_HANDLE) vkDestroyImageView(m_Device, l_Resource.View, nullptr);
            if (l_Resource.OwnsImage && l_Resource.Image != VK_NULL_HANDLE) vmaDestroyImage(m_Allocator.GetHandle(), l_Resource.Image, l_Resource.Allocation);
        }
    }

    void VulkanDevice::DestroySampler(SamplerHandle handle)
    {
        VulkanSamplerResource l_Resource{};
        if (m_Samplers.Free(handle, l_Resource) && l_Resource.Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device, l_Resource.Sampler, nullptr);
        }
    }

    void VulkanDevice::DestroyShader(ShaderHandle handle)
    {
        VulkanShaderResource l_Resource{};
        if (m_Shaders.Free(handle, l_Resource) && l_Resource.Module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device, l_Resource.Module, nullptr);
        }
    }

    void VulkanDevice::DestroyPipeline(PipelineHandle handle)
    {
        VulkanPipelineResource l_Resource{};
        if (m_Pipelines.Free(handle, l_Resource))
        {
            if (l_Resource.Pipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_Device, l_Resource.Pipeline, nullptr);
            if (l_Resource.Layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_Device, l_Resource.Layout, nullptr);
        }
    }

    void VulkanDevice::UpdateBuffer(BufferHandle, const void*, uint64_t, uint64_t)
    {
        TR_CORE_WARN("VulkanDevice: UpdateBuffer not yet implemented");
    }

    std::unique_ptr<Swapchain> VulkanDevice::CreateSwapchain(const SwapchainDescription& description)
    {
        auto l_Swapchain = std::make_unique<VulkanSwapchain>(*this, description);
        if (!l_Swapchain->Initialize())
        {
            return nullptr;
        }

        return l_Swapchain;
    }

    std::unique_ptr<CommandList> VulkanDevice::CreateCommandList()
    {
        TR_CORE_WARN("VulkanDevice: CreateCommandList not yet implemented");
        return nullptr;
    }

    void VulkanDevice::Submit(CommandList&)
    {
        TR_CORE_WARN("VulkanDevice: Submit not yet implemented");
    }

    void VulkanDevice::WaitIdle()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);
        }
    }

    TextureHandle VulkanDevice::RegisterExternalTexture(VkImage image, VkImageView view, VkFormat format, const VkExtent3D& extent, VkImageAspectFlags aspect)
    {
        VulkanTextureResource l_Resource{};
        l_Resource.Image = image;
        l_Resource.View = view;
        l_Resource.Format = format;
        l_Resource.Extent = extent;
        l_Resource.Aspect = aspect;
        l_Resource.OwnsImage = false;
        l_Resource.OwnsView = false;

        return m_Textures.Allocate(l_Resource);
    }

    void VulkanDevice::QueryCapabilities()
    {
        const VkPhysicalDeviceProperties& l_Properties = m_PhysicalDevice.GetProperties();

        m_Capabilities.DeviceName = l_Properties.deviceName;
        m_Capabilities.MaxTexture2DSize = l_Properties.limits.maxImageDimension2D;
        m_Capabilities.MaxPushConstantSize = l_Properties.limits.maxPushConstantsSize;
        m_Capabilities.MaxColorAttachments = l_Properties.limits.maxColorAttachments;

        VkPhysicalDeviceFeatures l_Features{};
        vkGetPhysicalDeviceFeatures(m_PhysicalDevice.GetHandle(), &l_Features);
        m_Capabilities.SupportsAnisotropy = l_Features.samplerAnisotropy == VK_TRUE;
        m_Capabilities.SupportsRayTracing = false;

        VkPhysicalDeviceMemoryProperties l_Memory{};
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice.GetHandle(), &l_Memory);

        uint64_t l_LocalMemory = 0;
        for (uint32_t l_Index = 0; l_Index < l_Memory.memoryHeapCount; ++l_Index)
        {
            if ((l_Memory.memoryHeaps[l_Index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
            {
                l_LocalMemory += l_Memory.memoryHeaps[l_Index].size;
            }
        }

        m_Capabilities.DedicatedVideoMemory = l_LocalMemory;
    }

    bool VulkanDevice::CreateLogicalDevice()
    {
        const QueueFamilyIndices& l_Families = m_PhysicalDevice.GetQueueFamilies();
        m_GraphicsQueueFamily = l_Families.Graphics.value();
        m_PresentQueueFamily = l_Families.Present.value();

        std::set<uint32_t> l_UniqueFamilies = { m_GraphicsQueueFamily, m_PresentQueueFamily };

        float l_Priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> l_QueueInfos;
        l_QueueInfos.reserve(l_UniqueFamilies.size());

        for (uint32_t l_Family : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_QueueCreateInfo{};
            l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_QueueCreateInfo.queueFamilyIndex = l_Family;
            l_QueueCreateInfo.queueCount = 1;
            l_QueueCreateInfo.pQueuePriorities = &l_Priority;
            l_QueueInfos.push_back(l_QueueCreateInfo);
        }

        VkPhysicalDeviceVulkan13Features l_Vulkan13Features{};
        l_Vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        l_Vulkan13Features.dynamicRendering = VK_TRUE;
        l_Vulkan13Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceVulkan12Features l_Vulkan12Features{};
        l_Vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        l_Vulkan12Features.timelineSemaphore = VK_TRUE;
        l_Vulkan12Features.pNext = &l_Vulkan13Features;

        VkPhysicalDeviceFeatures2 l_Features{};
        l_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        l_Features.pNext = &l_Vulkan12Features;

        const std::vector<const char*>& l_Extensions = m_PhysicalDevice.GetRequiredExtensions();

        VkDeviceCreateInfo l_DeviceCreateInfo{};
        l_DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_DeviceCreateInfo.pNext = &l_Features;
        l_DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueInfos.size());
        l_DeviceCreateInfo.pQueueCreateInfos = l_QueueInfos.data();
        l_DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_DeviceCreateInfo.ppEnabledExtensionNames = l_Extensions.data();

        VkResult l_Result = vkCreateDevice(m_PhysicalDevice.GetHandle(), &l_DeviceCreateInfo, nullptr, &m_Device);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("VulkanDevice: vkCreateDevice failed ({})", static_cast<int>(l_Result));
            return false;
        }

        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);

        return true;
    }
}