#include <Trinity/Renderer/Textures/Image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    std::optional<ImageData> Image::Load(const std::filesystem::path& path, bool srgb)
    {
        const std::string l_PathString = path.string();

        int l_Width = 0;
        int l_Height = 0;
        int l_Channels = 0;

        ImageData l_Image;

        if (stbi_is_hdr(l_PathString.c_str()))
        {
            float* l_Data = stbi_loadf(l_PathString.c_str(), &l_Width, &l_Height, &l_Channels, 4);
            if (l_Data == nullptr)
            {
                ("Image: failed to load HDR '{}' ({})", l_PathString, stbi_failure_reason());
                return std::nullopt;
            }

            const size_t l_ByteCount = static_cast<size_t>(l_Width) * static_cast<size_t>(l_Height) * 4 * sizeof(float);
            l_Image.Pixels.resize(l_ByteCount);
            std::memcpy(l_Image.Pixels.data(), l_Data, l_ByteCount);
            stbi_image_free(l_Data);

            l_Image.IsHDR = true;
            l_Image.SuggestedFormat = Format::RGBA32_SFLOAT;
        }
        else
        {
            stbi_uc* l_Data = stbi_load(l_PathString.c_str(), &l_Width, &l_Height, &l_Channels, 4);
            if (l_Data == nullptr)
            {
                ("Image: failed to load '{}' ({})", l_PathString, stbi_failure_reason());
                return std::nullopt;
            }

            const size_t l_ByteCount = static_cast<size_t>(l_Width) * static_cast<size_t>(l_Height) * 4;
            l_Image.Pixels.resize(l_ByteCount);
            std::memcpy(l_Image.Pixels.data(), l_Data, l_ByteCount);
            stbi_image_free(l_Data);

            l_Image.IsHDR = false;
            l_Image.SuggestedFormat = srgb ? Format::RGBA8_SRGB : Format::RGBA8_UNORM;
        }

        l_Image.Width = static_cast<uint32_t>(l_Width);
        l_Image.Height = static_cast<uint32_t>(l_Height);
        l_Image.Channels = 4;

        ("Image: loaded '{}' ({}x{}, {})", l_PathString, l_Image.Width, l_Image.Height, l_Image.IsHDR ? "HDR" : "LDR");

        return l_Image;
    }
}