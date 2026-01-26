#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Engine
{
    class VulkanDebugUtils
    {
    public:
        VulkanDebugUtils() = default;
        ~VulkanDebugUtils() = default;

        VulkanDebugUtils(const VulkanDebugUtils&) = delete;
        VulkanDebugUtils& operator=(const VulkanDebugUtils&) = delete;
        VulkanDebugUtils(VulkanDebugUtils&&) = delete;
        VulkanDebugUtils& operator=(VulkanDebugUtils&&) = delete;

        void Initialize(VkInstance instance, VkDevice device);
        void Shutdown();

        void SetObjectName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name) const;

        class ScopedCmdLabel
        {
        public:
            ScopedCmdLabel(const VulkanDebugUtils& debugUtils, VkCommandBuffer commandBuffer, const char* labelName);
            ScopedCmdLabel(const VulkanDebugUtils& debugUtils, VkCommandBuffer commandBuffer, const char* labelName, const float* color);
            ~ScopedCmdLabel();

            ScopedCmdLabel(const ScopedCmdLabel&) = delete;
            ScopedCmdLabel& operator=(const ScopedCmdLabel&) = delete;

        private:
            const VulkanDebugUtils* m_DebugUtils = nullptr;
            VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        };

    private:
        void BeginCmdLabel(VkCommandBuffer commandBuffer, const char* labelName, const float* color) const;
        void EndCmdLabel(VkCommandBuffer commandBuffer) const;

    private:
        PFN_vkSetDebugUtilsObjectNameEXT m_SetDebugUtilsObjectName = nullptr;
        PFN_vkCmdBeginDebugUtilsLabelEXT m_BeginDebugUtilsLabel = nullptr;
        PFN_vkCmdEndDebugUtilsLabelEXT m_EndDebugUtilsLabel = nullptr;
        bool m_Enabled = false;
    };
}