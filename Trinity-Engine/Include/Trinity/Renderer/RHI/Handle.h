#pragma once

#include <cstdint>
#include <functional>

namespace Trinity
{
    template<typename T>
    class Handle
    {
    public:
        static constexpr uint32_t InvalidIndex = 0xFFFFFFFF;

        Handle() = default;

        Handle(uint32_t index, uint32_t generation) : m_Index(index), m_Generation(generation)
        {

        }

        uint32_t GetIndex() const { return m_Index; }
        uint32_t GetGeneration() const { return m_Generation; }

        bool IsValid() const { return m_Index != InvalidIndex; }

        uint64_t Pack() const { return (static_cast<uint64_t>(m_Generation) << 32) | static_cast<uint64_t>(m_Index); }

        bool operator==(const Handle& other) const { return m_Index == other.m_Index && m_Generation == other.m_Generation; }
        bool operator!=(const Handle& other) const { return !(*this == other); }

    private:
        uint32_t m_Index = InvalidIndex;
        uint32_t m_Generation = 0;
    };

    struct BufferTag
    {

    };
    
    struct TextureTag
    {

    };
    
    struct SamplerTag
    {

    };

    struct ShaderTag
    {

    };
    
    struct PipelineTag
    {

    };
    
    struct RenderTargetTag
    {

    };

    using BufferHandle = Handle<BufferTag>;
    using TextureHandle = Handle<TextureTag>;
    using SamplerHandle = Handle<SamplerTag>;
    using ShaderHandle = Handle<ShaderTag>;
    using PipelineHandle = Handle<PipelineTag>;
    using RenderTargetHandle = Handle<RenderTargetTag>;
}

namespace std
{
    template<typename T>
    struct hash<Trinity::Handle<T>>
    {
        std::size_t operator()(const Trinity::Handle<T>& handle) const noexcept { return hash<uint64_t>()(handle.Pack()); }
    };
}