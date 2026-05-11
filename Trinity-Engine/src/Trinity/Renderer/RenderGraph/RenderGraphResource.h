#pragma once

#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <limits>
#include <string>

namespace Trinity
{
	struct RenderGraphResourceHandle
	{
		uint32_t Index = std::numeric_limits<uint32_t>::max();

		bool IsValid() const { return Index != std::numeric_limits<uint32_t>::max(); }

		bool operator==(const RenderGraphResourceHandle& other) const { return Index == other.Index; }
		bool operator!=(const RenderGraphResourceHandle& other) const { return Index != other.Index; }
	};

    struct RenderGraphTextureDescription
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t MipLevels = 1;
        uint32_t ArrayLayers = 1;
        TextureFormat Format = TextureFormat::None;
        TextureUsage Usage = TextureUsage::None;
        bool Persistent = false;
        std::string DebugName;
    };
}