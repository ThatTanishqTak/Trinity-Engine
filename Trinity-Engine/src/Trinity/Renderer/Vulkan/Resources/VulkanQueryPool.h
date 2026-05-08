#pragma once

#include "Trinity/Renderer/Resources/QueryPool.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanQueryPool final : public QueryPool
    {
    public:
        VulkanQueryPool(VkDevice device, const VkPhysicalDeviceProperties& deviceProperties, const QueryPoolSpecification& specification);
        ~VulkanQueryPool() override;

        void Reset(uint32_t firstQuery, uint32_t queryCount) override;
        bool GetResults(uint32_t firstQuery, uint32_t queryCount, std::vector<uint64_t>& outResults) override;

        uint32_t GetCount() const override { return m_Specification.Count; }
        QueryType GetType() const override { return m_Specification.Type; }
        float GetTimestampPeriod() const override { return m_TimestampPeriod; }

        VkQueryPool GetVkPool() const { return m_Pool; }
        uint32_t GetResultsPerQuery() const { return m_ResultsPerQuery; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkQueryPool m_Pool = VK_NULL_HANDLE;
        float m_TimestampPeriod = 0.0f;
        uint32_t m_ResultsPerQuery = 1;
    };
}