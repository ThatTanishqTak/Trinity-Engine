#pragma once

#include "Trinity/Renderer/Resources/DescriptorSetLayout.h"
#include "Trinity/Renderer/Resources/Shader.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    struct ComputePushConstantRange
    {
        uint32_t Offset = 0;
        uint32_t Size = 0;
    };

    struct ComputePipelineSpecification
    {
        std::shared_ptr<Shader> PipelineShader;
        std::vector<std::shared_ptr<DescriptorSetLayout>> DescriptorSetLayouts;
        std::vector<ComputePushConstantRange> PushConstants;

        uint32_t WorkgroupSizeX = 1;
        uint32_t WorkgroupSizeY = 1;
        uint32_t WorkgroupSizeZ = 1;

        std::string DebugName;
    };

    class ComputePipeline
    {
    public:
        virtual ~ComputePipeline() = default;

        const ComputePipelineSpecification& GetSpecification() const { return m_Specification; }

    protected:
        ComputePipelineSpecification m_Specification;
    };
}