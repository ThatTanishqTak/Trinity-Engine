#include "Trinity/Renderer/Vulkan/VulkanPipelineCache.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <cstring>
#include <fstream>
#include <vector>

namespace Trinity
{
    void VulkanPipelineCache::Initialize(VkDevice device, const VkPhysicalDeviceProperties& properties, const std::string& path)
    {
        TR_CORE_TRACE("Initializing Vulkan Pipeline Cache");

        m_Device = device;
        m_Path = path;

        std::vector<char> l_InitialData;

        if (!m_Path.empty())
        {
            std::ifstream l_File(m_Path, std::ios::binary | std::ios::ate);
            if (l_File.is_open())
            {
                const std::streamsize l_Size = l_File.tellg();
                if (l_Size > 0)
                {
                    l_File.seekg(0, std::ios::beg);
                    l_InitialData.resize(static_cast<size_t>(l_Size));
                    l_File.read(l_InitialData.data(), l_Size);

                    if (!IsHeaderCompatible(l_InitialData.data(), l_InitialData.size(), properties))
                    {
                        TR_CORE_WARN("Pipeline cache at '{}' is incompatible with current device; discarding", m_Path);
                        l_InitialData.clear();
                    }
                    else
                    {
                        TR_CORE_TRACE("Loaded pipeline cache from '{}' ({} bytes)", m_Path, l_InitialData.size());
                    }
                }
            }
            else
            {
                TR_CORE_TRACE("No existing pipeline cache at '{}'; starting fresh", m_Path);
            }
        }

        VkPipelineCacheCreateInfo l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        l_CreateInfo.initialDataSize = l_InitialData.size();
        l_CreateInfo.pInitialData = l_InitialData.empty() ? nullptr : l_InitialData.data();

        VulkanUtilities::VKCheck(vkCreatePipelineCache(m_Device, &l_CreateInfo, nullptr, &m_Cache), "Failed to create pipeline cache");

        TR_CORE_TRACE("Vulkan Pipeline Cache Initialized");
    }

    void VulkanPipelineCache::Shutdown()
    {
        if (m_Cache == VK_NULL_HANDLE)
        {
            return;
        }

        TR_CORE_TRACE("Shutting Down Vulkan Pipeline Cache");

        if (!m_Path.empty())
        {
            SaveToDisk();
        }

        vkDestroyPipelineCache(m_Device, m_Cache, nullptr);
        m_Cache = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_Path.clear();

        TR_CORE_TRACE("Vulkan Pipeline Cache Shut Down");
    }

    bool VulkanPipelineCache::IsHeaderCompatible(const void* data, size_t size, const VkPhysicalDeviceProperties& properties) const
    {
        if (size < 32)
        {
            return false;
        }

        const uint8_t* l_Bytes = static_cast<const uint8_t*>(data);

        uint32_t l_HeaderLength = 0;
        uint32_t l_HeaderVersion = 0;
        uint32_t l_VendorID = 0;
        uint32_t l_DeviceID = 0;

        std::memcpy(&l_HeaderLength, l_Bytes + 0, sizeof(uint32_t));
        std::memcpy(&l_HeaderVersion, l_Bytes + 4, sizeof(uint32_t));
        std::memcpy(&l_VendorID, l_Bytes + 8, sizeof(uint32_t));
        std::memcpy(&l_DeviceID, l_Bytes + 12, sizeof(uint32_t));

        if (l_HeaderLength != 32)
        {
            return false;
        }

        if (l_HeaderVersion != static_cast<uint32_t>(VK_PIPELINE_CACHE_HEADER_VERSION_ONE))
        {
            return false;
        }

        if (l_VendorID != properties.vendorID)
        {
            return false;
        }

        if (l_DeviceID != properties.deviceID)
        {
            return false;
        }

        if (std::memcmp(l_Bytes + 16, properties.pipelineCacheUUID, VK_UUID_SIZE) != 0)
        {
            return false;
        }

        return true;
    }

    void VulkanPipelineCache::SaveToDisk() const
    {
        size_t l_DataSize = 0;
        VkResult l_QueryResult = vkGetPipelineCacheData(m_Device, m_Cache, &l_DataSize, nullptr);
        if (l_QueryResult != VK_SUCCESS || l_DataSize == 0)
        {
            TR_CORE_TRACE("Pipeline cache contains no data to save");
            return;
        }

        std::vector<char> l_Data(l_DataSize);
        VkResult l_FetchResult = vkGetPipelineCacheData(m_Device, m_Cache, &l_DataSize, l_Data.data());
        if (l_FetchResult != VK_SUCCESS)
        {
            TR_CORE_ERROR("vkGetPipelineCacheData failed during save");
            return;
        }

        std::ofstream l_File(m_Path, std::ios::binary | std::ios::trunc);
        if (!l_File.is_open())
        {
            TR_CORE_ERROR("Failed to open pipeline cache for writing: '{}'", m_Path);
            return;
        }

        l_File.write(l_Data.data(), static_cast<std::streamsize>(l_DataSize));
        if (!l_File.good())
        {
            TR_CORE_ERROR("Failed to write pipeline cache to '{}'", m_Path);
            return;
        }

        TR_CORE_TRACE("Pipeline cache saved to '{}' ({} bytes)", m_Path, l_DataSize);
    }
}