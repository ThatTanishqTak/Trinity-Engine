#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine
{
    class VulkanDevice;

    class VulkanDescriptors
    {
    public:
        VulkanDescriptors() = default;
        ~VulkanDescriptors() = default;

        VulkanDescriptors(const VulkanDescriptors&) = delete;
        VulkanDescriptors& operator=(const VulkanDescriptors&) = delete;
        VulkanDescriptors(VulkanDescriptors&&) = delete;
        VulkanDescriptors& operator=(VulkanDescriptors&&) = delete;

        void Initialize(VulkanDevice& device, uint32_t framesInFlight);
        void Shutdown(VulkanDevice& device);
        void Shutdown(VulkanDevice& device, const std::function<void(std::function<void()>&&)>& submitResourceFree);
        void OnBeginFrame(VulkanDevice& device, uint32_t frameIndex);

        bool IsValid() const { return m_GlobalSetLayout != VK_NULL_HANDLE && m_MaterialSetLayout != VK_NULL_HANDLE && !m_Pools.empty(); }

        VkDescriptorSetLayout GetGlobalSetLayout() const { return m_GlobalSetLayout; }
        VkDescriptorSetLayout GetMaterialSetLayout() const { return m_MaterialSetLayout; }

        VkDescriptorSet AllocateGlobalSet(uint32_t frameIndex) const;
        VkDescriptorSet AllocateMaterialSet(uint32_t frameIndex) const;

        void WriteBuffer(VkDescriptorSet descriptorSet, uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType) const;
        void WriteImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, const VkDescriptorImageInfo& imageInfo) const;

        static void SetPassCreationInProgress(bool value);
        static void SetPassRecordInProgress(bool value);
        static bool IsPassCreationInProgress();
        static bool IsPassRecordInProgress();

    private:
        VulkanDevice* m_Device = nullptr;
        VkDescriptorSetLayout m_GlobalSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_MaterialSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorPool> m_Pools;
        uint32_t m_FramesInFlight = 0;
    };
}