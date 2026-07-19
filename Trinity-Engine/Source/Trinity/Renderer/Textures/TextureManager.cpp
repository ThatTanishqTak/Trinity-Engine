#include <Trinity/Renderer/Textures/TextureManager.h>

#include <array>
#include <optional>

#include <Trinity/Renderer/Textures/Image.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    TextureManager::TextureManager(GraphicsDevice& device, FileSystem& fileSystem) : m_Device(device), m_FileSystem(fileSystem)
    {

    }

    TextureManager::~TextureManager()
    {
        Shutdown();
    }

    TextureHandle TextureManager::CreateFromPixels(const void* pixels, uint64_t size, uint32_t width, uint32_t height, Format format, bool generateMips, const std::string& debugName)
    {
        TextureDescription l_Description;
        l_Description.Type = TextureType::Texture2D;
        l_Description.Format = format;
        l_Description.Usage = TextureUsage::Sampled;
        l_Description.Width = width;
        l_Description.Height = height;
        l_Description.Depth = 1;
        l_Description.MipLevels = 1;
        l_Description.ArrayLayers = 1;
        l_Description.SampleCount = 1;
        l_Description.InitialData = pixels;
        l_Description.InitialDataSize = size;
        l_Description.GenerateMips = generateMips;
        l_Description.DebugName = debugName;

        return m_Device.CreateTexture(l_Description);
    }

    bool TextureManager::Initialize()
    {
        const uint8_t l_WhitePixel[4] = { 255, 255, 255, 255 };
        const uint8_t l_BlackPixel[4] = { 0, 0, 0, 255 };
        const uint8_t l_NormalPixel[4] = { 128, 128, 255, 255 };

        m_White = CreateFromPixels(l_WhitePixel, sizeof(l_WhitePixel), 1, 1, Format::RGBA8_SRGB, false, "Fallback.White");
        m_Black = CreateFromPixels(l_BlackPixel, sizeof(l_BlackPixel), 1, 1, Format::RGBA8_SRGB, false, "Fallback.Black");
        m_Normal = CreateFromPixels(l_NormalPixel, sizeof(l_NormalPixel), 1, 1, Format::RGBA8_UNORM, false, "Fallback.Normal");

        const uint32_t l_ErrorSize = 16;
        std::array<uint8_t, l_ErrorSize* l_ErrorSize * 4> l_ErrorPixels{};
        for (uint32_t l_Y = 0; l_Y < l_ErrorSize; ++l_Y)
        {
            for (uint32_t l_X = 0; l_X < l_ErrorSize; ++l_X)
            {
                const bool l_Magenta = (((l_X / 4) + (l_Y / 4)) % 2) == 0;
                const size_t l_Index = (static_cast<size_t>(l_Y) * l_ErrorSize + l_X) * 4;
                l_ErrorPixels[l_Index + 0] = l_Magenta ? 255 : 0;
                l_ErrorPixels[l_Index + 1] = 0;
                l_ErrorPixels[l_Index + 2] = l_Magenta ? 255 : 0;
                l_ErrorPixels[l_Index + 3] = 255;
            }
        }

        m_Error = CreateFromPixels(l_ErrorPixels.data(), l_ErrorPixels.size(), l_ErrorSize, l_ErrorSize, Format::RGBA8_SRGB, false, "Fallback.Error");

        if (!m_White.IsValid() || !m_Black.IsValid() || !m_Normal.IsValid() || !m_Error.IsValid())
        {


            return false;
        }

        SamplerDescription l_SamplerDescription;
        l_SamplerDescription.LinearFilter = true;
        l_SamplerDescription.LinearMipmap = true;
        l_SamplerDescription.RepeatU = true;
        l_SamplerDescription.RepeatV = true;
        l_SamplerDescription.RepeatW = true;
        l_SamplerDescription.MaxAnisotropy = 1.0f;
        l_SamplerDescription.DebugName = "DefaultSampler";

        m_DefaultSampler = m_Device.CreateSampler(l_SamplerDescription);
        if (!m_DefaultSampler.IsValid())
        {


            return false;
        }



        return true;
    }

    void TextureManager::Shutdown()
    {
        for (auto& it_Entry : m_Cache)
        {
            if (it_Entry.second.IsValid())
            {
                m_Device.DestroyTexture(it_Entry.second);
            }
        }
        m_Cache.clear();

        if (m_DefaultSampler.IsValid())
        {
            m_Device.DestroySampler(m_DefaultSampler);
            m_DefaultSampler = SamplerHandle{};
        }

        if (m_Error.IsValid())
        {
            m_Device.DestroyTexture(m_Error);
            m_Error = TextureHandle{};
        }

        if (m_Normal.IsValid())
        {
            m_Device.DestroyTexture(m_Normal);
            m_Normal = TextureHandle{};
        }

        if (m_Black.IsValid())
        {
            m_Device.DestroyTexture(m_Black);
            m_Black = TextureHandle{};
        }

        if (m_White.IsValid())
        {
            m_Device.DestroyTexture(m_White);
            m_White = TextureHandle{};
        }
    }

    TextureHandle TextureManager::Load(const std::filesystem::path& relativePath, bool srgb, bool generateMips)
    {
        const std::string l_Key = relativePath.generic_string();

        auto it_Cached = m_Cache.find(l_Key);
        if (it_Cached != m_Cache.end())
        {
            return it_Cached->second;
        }

        std::filesystem::path l_FullPath = m_FileSystem.Resolve(BaseDirectory::Executable, relativePath);
        std::optional<ImageData> l_Image = Image::Load(l_FullPath, srgb);
        if (!l_Image)
        {


            return m_Error;
        }

        TextureHandle l_Handle = CreateFromPixels(l_Image->Pixels.data(), l_Image->Pixels.size(), l_Image->Width, l_Image->Height, l_Image->SuggestedFormat, generateMips, l_Key);
        if (!l_Handle.IsValid())
        {


            return m_Error;
        }

        m_Cache[l_Key] = l_Handle;

        return l_Handle;
    }
}