#include "Engine/Renderer/Vulkan/VulkanDebugUtils.h"

#include <cstring>

namespace Engine
{
    void VulkanDebugUtils::Initialize(VkInstance instance, VkDevice device)
    {
#if defined(ENGINE_DEBUG) || defined(TR_ENABLE_VK_DEBUG_UTILS)
        m_Enabled = true;
#else
        m_Enabled = false;
#endif

        if (!m_Enabled || instance == VK_NULL_HANDLE || device == VK_NULL_HANDLE)
        {
            return;
        }

        m_SetDebugUtilsObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
        if (!m_SetDebugUtilsObjectName)
        {
            m_SetDebugUtilsObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
        }

        m_BeginDebugUtilsLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));
        if (!m_BeginDebugUtilsLabel)
        {
            m_BeginDebugUtilsLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
        }

        m_EndDebugUtilsLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
        if (!m_EndDebugUtilsLabel)
        {
            m_EndDebugUtilsLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
        }
    }

    void VulkanDebugUtils::Shutdown()
    {
        m_SetDebugUtilsObjectName = nullptr;
        m_BeginDebugUtilsLabel = nullptr;
        m_EndDebugUtilsLabel = nullptr;
        m_Enabled = false;
    }

    void VulkanDebugUtils::SetObjectName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name) const
    {
        if (!m_Enabled || !m_SetDebugUtilsObjectName || device == VK_NULL_HANDLE || objectHandle == 0 || !name)
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
        l_NameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_NameInfo.objectType = objectType;
        l_NameInfo.objectHandle = objectHandle;
        l_NameInfo.pObjectName = name;

        m_SetDebugUtilsObjectName(device, &l_NameInfo);
    }

    VulkanDebugUtils::ScopedCmdLabel::ScopedCmdLabel(const VulkanDebugUtils& debugUtils, VkCommandBuffer commandBuffer, const char* labelName) : m_DebugUtils(&debugUtils), 
        m_CommandBuffer(commandBuffer)
    {
        m_DebugUtils->BeginCmdLabel(m_CommandBuffer, labelName, nullptr);
    }

    VulkanDebugUtils::ScopedCmdLabel::ScopedCmdLabel(const VulkanDebugUtils& debugUtils, VkCommandBuffer commandBuffer, const char* labelName, const float* color) 
        : m_DebugUtils(&debugUtils), m_CommandBuffer(commandBuffer)
    {
        m_DebugUtils->BeginCmdLabel(m_CommandBuffer, labelName, color);
    }

    VulkanDebugUtils::ScopedCmdLabel::~ScopedCmdLabel()
    {
        if (m_DebugUtils)
        {
            m_DebugUtils->EndCmdLabel(m_CommandBuffer);
        }
    }

    void VulkanDebugUtils::BeginCmdLabel(VkCommandBuffer commandBuffer, const char* labelName, const float* color) const
    {
        if (!m_Enabled || !m_BeginDebugUtilsLabel || commandBuffer == VK_NULL_HANDLE || !labelName)
        {
            return;
        }

        VkDebugUtilsLabelEXT l_Label{};
        l_Label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        l_Label.pLabelName = labelName;
        if (color)
        {
            std::memcpy(l_Label.color, color, sizeof(l_Label.color));
        }

        m_BeginDebugUtilsLabel(commandBuffer, &l_Label);
    }

    void VulkanDebugUtils::EndCmdLabel(VkCommandBuffer commandBuffer) const
    {
        if (!m_Enabled || !m_EndDebugUtilsLabel || commandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        m_EndDebugUtilsLabel(commandBuffer);
    }
}