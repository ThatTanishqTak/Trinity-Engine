#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <Trinity/Platform/PlatformTypes.h>
#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanInstance.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanSurface.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanPhysicalDevice.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanAllocator.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanCommands.h>

namespace Trinity
{
    class VulkanSwapchain;

    struct VulkanBufferResource
    {
        VkBuffer Buffer = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        uint64_t Size = 0;
        BufferUsage Usage = BufferUsage::None;
        MemoryUsage Memory = MemoryUsage::GpuOnly;
        void* Mapped = nullptr;
        std::string DebugName;
    };

    struct VulkanTextureResource
    {
        VkImage Image = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VkImageView View = VK_NULL_HANDLE;
        VkFormat Format = VK_FORMAT_UNDEFINED;
        VkExtent3D Extent{};
        VkImageAspectFlags Aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        bool OwnsImage = true;
        bool OwnsView = true;
        ResourceState CurrentState = ResourceState::Undefined;
        std::string DebugName;
    };

    struct VulkanSamplerResource
    {
        VkSampler Sampler = VK_NULL_HANDLE;
        std::string DebugName;
    };

    struct VulkanShaderResource
    {
        VkShaderModule Module = VK_NULL_HANDLE;
        ShaderStage Stage = ShaderStage::None;
        std::string EntryPoint = "main";
        std::string DebugName;
    };

    struct VulkanPipelineResource
    {
        VkPipeline Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout Layout = VK_NULL_HANDLE;
        std::string DebugName;
    };

    struct DeferredRelease
    {
        enum class Kind
        {
            Buffer,
            Texture,
            Sampler,
            Shader,
            Pipeline
        };

        uint64_t Frame = 0;
        Kind Type = Kind::Buffer;

        VkBuffer Buffer = VK_NULL_HANDLE;
        VkImage Image = VK_NULL_HANDLE;
        VkImageView View = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VkSampler Sampler = VK_NULL_HANDLE;
        VkShaderModule Module = VK_NULL_HANDLE;
        VkPipeline Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout Layout = VK_NULL_HANDLE;

        bool OwnsImage = true;
        bool OwnsView = true;
    };

    template<typename Payload, typename Tag>
    class VulkanResourcePool
    {
    public:
        Handle<Tag> Allocate(const Payload& payload)
        {
            uint32_t l_Index;

            if (!m_FreeList.empty())
            {
                l_Index = m_FreeList.back();
                m_FreeList.pop_back();
                m_Slots[l_Index].Data = payload;
                m_Slots[l_Index].Alive = true;
            }
            else
            {
                l_Index = static_cast<uint32_t>(m_Slots.size());
                m_Slots.push_back({ 1, true, payload });
            }

            return Handle<Tag>(l_Index, m_Slots[l_Index].Generation);
        }

        Payload* Get(Handle<Tag> handle)
        {
            if (!handle.IsValid())
            {
                return nullptr;
            }

            uint32_t l_Index = handle.GetIndex();
            if (l_Index >= m_Slots.size())
            {
                return nullptr;
            }

            Slot& l_Slot = m_Slots[l_Index];
            if (!l_Slot.Alive || l_Slot.Generation != handle.GetGeneration())
            {
                return nullptr;
            }

            return &l_Slot.Data;
        }

        bool Free(Handle<Tag> handle, Payload& outPayload)
        {
            Payload* l_Payload = Get(handle);
            if (l_Payload == nullptr)
            {
                return false;
            }

            outPayload = *l_Payload;
            uint32_t l_Index = handle.GetIndex();
            m_Slots[l_Index].Alive = false;
            ++m_Slots[l_Index].Generation;
            m_FreeList.push_back(l_Index);

            return true;
        }

        template<typename Fn>
        void ForEachAlive(Fn&& function)
        {
            for (Slot& it_Slot : m_Slots)
            {
                if (it_Slot.Alive)
                {
                    function(it_Slot.Data);
                }
            }
        }

    private:
        struct Slot
        {
            uint32_t Generation = 1;
            bool Alive = false;
            Payload Data{};
        };

        std::vector<Slot> m_Slots;
        std::vector<uint32_t> m_FreeList;
    };

    class VulkanDevice : public GraphicsDevice
    {
    public:
        VulkanDevice(const NativeWindowHandle& window, const std::string& applicationName, bool enableValidation);
        ~VulkanDevice() override;

        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;

        bool Initialize() override;
        void Shutdown() override;

        GraphicsBackend GetBackend() const override { return GraphicsBackend::Vulkan; }
        const DeviceCapabilities& GetCapabilities() const override { return m_Capabilities; }

        BufferHandle CreateBuffer(const BufferDescription& description) override;
        TextureHandle CreateTexture(const TextureDescription& description) override;
        SamplerHandle CreateSampler(const SamplerDescription& description) override;
        ShaderHandle CreateShader(const ShaderDescription& description) override;
        PipelineHandle CreatePipeline(const PipelineDescription& description) override;

        void DestroyBuffer(BufferHandle handle) override;
        void DestroyTexture(TextureHandle handle) override;
        void DestroySampler(SamplerHandle handle) override;
        void DestroyShader(ShaderHandle handle) override;
        void DestroyPipeline(PipelineHandle handle) override;

        void UpdateBuffer(BufferHandle handle, const void* data, uint64_t size, uint64_t offset = 0) override;

        std::unique_ptr<Swapchain> CreateSwapchain(const SwapchainDescription& description) override;
        std::unique_ptr<CommandList> CreateCommandList() override;

        void Submit(CommandList& commandList) override;
        void WaitIdle() override;

        void CollectGarbage() override;

        TextureHandle RegisterExternalTexture(VkImage image, VkImageView view, VkFormat format, const VkExtent3D& extent, VkImageAspectFlags aspect);
        void SetActiveSwapchain(VulkanSwapchain* swapchain) { m_ActiveSwapchain = swapchain; }
        VulkanSwapchain* GetActiveSwapchain() const { return m_ActiveSwapchain; }

        VulkanBufferResource* GetBuffer(BufferHandle handle) { return m_Buffers.Get(handle); }
        VulkanTextureResource* GetTexture(TextureHandle handle) { return m_Textures.Get(handle); }
        VulkanPipelineResource* GetPipeline(PipelineHandle handle) { return m_Pipelines.Get(handle); }

        ResourceState GetTextureState(TextureHandle handle)
        {
            VulkanTextureResource* l_Texture = m_Textures.Get(handle);

            return l_Texture != nullptr ? l_Texture->CurrentState : ResourceState::Undefined;
        }

        VkDevice GetHandle() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice.GetHandle(); }
        VkInstance GetInstance() const { return m_Instance.GetHandle(); }
        VkSurfaceKHR GetSurface() const { return m_Surface.GetHandle(); }
        VulkanAllocator& GetAllocator() { return m_Allocator; }
        VulkanCommands& GetCommands() { return m_Commands; }

        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
        uint32_t GetPresentQueueFamily() const { return m_PresentQueueFamily; }

    private:
        bool CreateLogicalDevice();
        void QueryCapabilities();
        void ReleaseNow(const DeferredRelease& release);
        void ReportLeaks();
        void SetObjectName(uint64_t handle, VkObjectType type, const std::string& name);

    private:
        NativeWindowHandle m_Window;
        std::string m_ApplicationName;
        bool m_EnableValidation = false;
        bool m_Initialized = false;

        VulkanInstance m_Instance;
        VulkanSurface m_Surface;
        VulkanPhysicalDevice m_PhysicalDevice;
        VulkanAllocator m_Allocator;
        VulkanCommands m_Commands;

        VkDevice m_Device = VK_NULL_HANDLE;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;

        uint32_t m_GraphicsQueueFamily = 0;
        uint32_t m_PresentQueueFamily = 0;

        DeviceCapabilities m_Capabilities;

        VulkanResourcePool<VulkanBufferResource, BufferTag> m_Buffers;
        VulkanResourcePool<VulkanTextureResource, TextureTag> m_Textures;
        VulkanResourcePool<VulkanSamplerResource, SamplerTag> m_Samplers;
        VulkanResourcePool<VulkanShaderResource, ShaderTag> m_Shaders;
        VulkanResourcePool<VulkanPipelineResource, PipelineTag> m_Pipelines;

        std::vector<DeferredRelease> m_DeferredReleases;
        uint64_t m_FrameCounter = 0;
        uint64_t m_DeferredFrameDelay = 3;

        PFN_vkSetDebugUtilsObjectNameEXT m_SetObjectName = nullptr;

        VulkanSwapchain* m_ActiveSwapchain = nullptr;
    };
}