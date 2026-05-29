#pragma once

#include <cstdint>
#include <string>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>

namespace Trinity
{
    enum class TextureType
    {
        Texture2D = 0,
        Texture2DArray,
        TextureCube,
        Texture3D
    };

    struct TextureDescription
    {
        TextureType Type = TextureType::Texture2D;
        Format Format = Format::RGBA8_UNORM;
        TextureUsage Usage = TextureUsage::Sampled;
        
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint32_t Depth = 1;
        uint32_t MipLevels = 1;
        uint32_t ArrayLayers = 1;
        uint32_t SampleCount = 1;
        uint64_t InitialDataSize = 0;

        const void* InitialData = nullptr;
        
        std::string DebugName;
    };

    struct SamplerDescription
    {
        bool LinearFilter = true;
        bool LinearMipmap = true;
        bool RepeatU = true;
        bool RepeatV = true;
        bool RepeatW = true;
        
        float MaxAnisotropy = 1.0f;

        std::string DebugName;
    };
}