#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/DescriptorSetLayout.h"
#include "Trinity/Renderer/Resources/Sampler.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>

namespace Trinity
{
    class DescriptorSet
    {
    public:
        virtual ~DescriptorSet() = default;

        virtual void WriteUniformBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) = 0;
        virtual void WriteStorageBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) = 0;
        virtual void WriteSampledImage(uint32_t binding, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Sampler>& sampler) = 0;
        virtual void WriteStorageImage(uint32_t binding, const std::shared_ptr<Texture>& texture) = 0;
        virtual void WriteSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler) = 0;

        virtual void WriteUniformBufferArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) = 0;
        virtual void WriteStorageBufferArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) = 0;
        virtual void WriteSampledImageArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Sampler>& sampler) = 0;
        virtual void WriteStorageImageArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Texture>& texture) = 0;

        virtual void Flush() = 0;

        virtual std::shared_ptr<DescriptorSetLayout> GetLayout() const = 0;
    };
}