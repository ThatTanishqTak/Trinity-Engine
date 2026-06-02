#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>

namespace Trinity
{
    struct ImageData
    {
        std::vector<uint8_t> Pixels;
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t Channels = 4;
        bool IsHDR = false;
        Format SuggestedFormat = Format::RGBA8_UNORM;
    };

    class Image
    {
    public:
        static std::optional<ImageData> Load(const std::filesystem::path& path, bool srgb);
    };
}