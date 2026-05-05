#pragma once

#include "Trinity/Renderer/Resources/Shader.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Trinity
{
    enum class DescriptorBindingType : uint8_t
    {
        UniformBuffer = 0,
        StorageBuffer,
        SampledImage,
        StorageImage,
        Sampler,
        CombinedImageSampler,
        UniformBufferDynamic,
        StorageBufferDynamic
    };

    enum class DescriptorBindingFlags : uint32_t
    {
        None = 0,
        PartiallyBound = 1 << 0,
        UpdateAfterBind = 1 << 1,
        VariableDescriptorCount = 1 << 2
    };

    inline DescriptorBindingFlags operator|(DescriptorBindingFlags a, DescriptorBindingFlags b) { return static_cast<DescriptorBindingFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
    inline bool operator&(DescriptorBindingFlags a, DescriptorBindingFlags b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }

    struct DescriptorBinding
    {
        uint32_t Binding = 0;
        DescriptorBindingType Type = DescriptorBindingType::UniformBuffer;
        ShaderStage Stage = ShaderStage::Fragment;
        uint32_t Count = 1;
        DescriptorBindingFlags Flags = DescriptorBindingFlags::None;
    };

    struct DescriptorSetLayoutSpecification
    {
        std::vector<DescriptorBinding> Bindings;
        std::string DebugName;
    };

    class DescriptorSetLayout
    {
    public:
        virtual ~DescriptorSetLayout() = default;

        const DescriptorSetLayoutSpecification& GetSpecification() const { return m_Specification; }

    protected:
        DescriptorSetLayoutSpecification m_Specification;
    };
}