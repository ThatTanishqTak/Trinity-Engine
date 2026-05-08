#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Trinity
{
    enum class QueryType : uint8_t
    {
        Timestamp = 0,
        Occlusion,
        PipelineStatistics
    };

    enum class PipelineStatisticFlags : uint32_t
    {
        None = 0,
        InputAssemblyVertices = 1 << 0,
        InputAssemblyPrimitives = 1 << 1,
        VertexShaderInvocations = 1 << 2,
        GeometryShaderInvocations = 1 << 3,
        GeometryShaderPrimitives = 1 << 4,
        ClippingInvocations = 1 << 5,
        ClippingPrimitives = 1 << 6,
        FragmentShaderInvocations = 1 << 7,
        TessControlShaderPatches = 1 << 8,
        TessEvalShaderInvocations = 1 << 9,
        ComputeShaderInvocations = 1 << 10
    };

    inline PipelineStatisticFlags operator|(PipelineStatisticFlags a, PipelineStatisticFlags b) { return static_cast<PipelineStatisticFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
    inline bool operator&(PipelineStatisticFlags a, PipelineStatisticFlags b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }

    struct QueryPoolSpecification
    {
        QueryType Type = QueryType::Timestamp;
        uint32_t Count = 0;
        PipelineStatisticFlags Statistics = PipelineStatisticFlags::None;
        std::string DebugName;
    };

    class QueryPool
    {
    public:
        virtual ~QueryPool() = default;

        virtual void Reset(uint32_t firstQuery, uint32_t queryCount) = 0;

        virtual bool GetResults(uint32_t firstQuery, uint32_t queryCount, std::vector<uint64_t>& outResults) = 0;

        virtual uint32_t GetCount() const = 0;
        virtual QueryType GetType() const = 0;
        virtual float GetTimestampPeriod() const = 0;

        const QueryPoolSpecification& GetSpecification() const { return m_Specification; }

    protected:
        QueryPoolSpecification m_Specification;
    };
}