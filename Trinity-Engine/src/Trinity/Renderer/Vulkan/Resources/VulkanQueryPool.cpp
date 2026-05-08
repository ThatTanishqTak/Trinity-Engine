#include "Trinity/Renderer/Vulkan/Resources/VulkanQueryPool.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    namespace
    {
        VkQueryType ToVkQueryType(QueryType type)
        {
            switch (type)
            {
                case QueryType::Timestamp:
                    return VK_QUERY_TYPE_TIMESTAMP;
                case QueryType::Occlusion:
                    return VK_QUERY_TYPE_OCCLUSION;
                case QueryType::PipelineStatistics:
                    return VK_QUERY_TYPE_PIPELINE_STATISTICS;
                default:
                    return VK_QUERY_TYPE_TIMESTAMP;
            }
        }

        VkQueryPipelineStatisticFlags ToVkPipelineStatistics(PipelineStatisticFlags flags)
        {
            VkQueryPipelineStatisticFlags l_Flags = 0;
            if (flags & PipelineStatisticFlags::InputAssemblyVertices)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT;
            }

            if (flags & PipelineStatisticFlags::InputAssemblyPrimitives)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
            }

            if (flags & PipelineStatisticFlags::VertexShaderInvocations)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT;
            }

            if (flags & PipelineStatisticFlags::GeometryShaderInvocations)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT;
            }

            if (flags & PipelineStatisticFlags::GeometryShaderPrimitives)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;
            }

            if (flags & PipelineStatisticFlags::ClippingInvocations)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
            }

            if (flags & PipelineStatisticFlags::ClippingPrimitives)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
            }

            if (flags & PipelineStatisticFlags::FragmentShaderInvocations)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
            }

            if (flags & PipelineStatisticFlags::TessControlShaderPatches)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT;
            }

            if (flags & PipelineStatisticFlags::TessEvalShaderInvocations)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
            }

            if (flags & PipelineStatisticFlags::ComputeShaderInvocations)
            {
                l_Flags |= VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
            }

            return l_Flags;
        }

        uint32_t CountSetBits(uint32_t value)
        {
            uint32_t l_Count = 0;
            while (value != 0)
            {
                value &= value - 1;
                l_Count++;
            }

            return l_Count;
        }
    }

    VulkanQueryPool::VulkanQueryPool(VkDevice device, const VkPhysicalDeviceProperties& deviceProperties, const QueryPoolSpecification& specification) : m_Device(device)
    {
        m_Specification = specification;
        m_TimestampPeriod = deviceProperties.limits.timestampPeriod;

        if (specification.Type == QueryType::PipelineStatistics)
        {
            m_ResultsPerQuery = CountSetBits(static_cast<uint32_t>(specification.Statistics));
            if (m_ResultsPerQuery == 0)
            {
                TR_CORE_ERROR("VulkanQueryPool [{}]: PipelineStatistics type requested but Statistics flags are empty", specification.DebugName);
                return;
            }
        }
        else
        {
            m_ResultsPerQuery = 1;
        }

        if (specification.Type == QueryType::Timestamp && deviceProperties.limits.timestampComputeAndGraphics == VK_FALSE)
        {
            TR_CORE_WARN("VulkanQueryPool [{}]: device does not advertise timestampComputeAndGraphics; timestamps may be invalid on some queues", specification.DebugName);
        }

        VkQueryPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        l_PoolInfo.queryType = ToVkQueryType(specification.Type);
        l_PoolInfo.queryCount = specification.Count;

        if (specification.Type == QueryType::PipelineStatistics)
        {
            l_PoolInfo.pipelineStatistics = ToVkPipelineStatistics(specification.Statistics);
        }

        VulkanUtilities::VKCheck(vkCreateQueryPool(m_Device, &l_PoolInfo, nullptr, &m_Pool), "Failed vkCreateQueryPool");

        Reset(0, specification.Count);
    }

    VulkanQueryPool::~VulkanQueryPool()
    {
        if (m_Pool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(m_Device, m_Pool, nullptr);
        }
    }

    void VulkanQueryPool::Reset(uint32_t firstQuery, uint32_t queryCount)
    {
        if (m_Pool == VK_NULL_HANDLE || queryCount == 0)
        {
            return;
        }

        vkResetQueryPool(m_Device, m_Pool, firstQuery, queryCount);
    }

    bool VulkanQueryPool::GetResults(uint32_t firstQuery, uint32_t queryCount, std::vector<uint64_t>& outResults)
    {
        if (m_Pool == VK_NULL_HANDLE || queryCount == 0)
        {
            return false;
        }

        const size_t l_TotalResults = static_cast<size_t>(queryCount) * m_ResultsPerQuery;
        outResults.resize(l_TotalResults);

        const VkDeviceSize l_Stride = m_ResultsPerQuery * sizeof(uint64_t);
        const VkDeviceSize l_DataSize = l_TotalResults * sizeof(uint64_t);

        VkResult l_Result = vkGetQueryPoolResults(m_Device, m_Pool, firstQuery, queryCount, l_DataSize, outResults.data(), l_Stride, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        if (l_Result == VK_NOT_READY)
        {
            return false;
        }

        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_ERROR("vkGetQueryPoolResults failed [{}]: VkResult={}", m_Specification.DebugName, static_cast<int>(l_Result));
            return false;
        }

        return true;
    }
}