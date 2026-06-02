#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>

namespace Trinity
{
    class FileSystem;

    class TextureManager
    {
    public:
        TextureManager(GraphicsDevice& device, FileSystem& fileSystem);
        ~TextureManager();

        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;

        bool Initialize();
        void Shutdown();

        TextureHandle Load(const std::filesystem::path& relativePath, bool srgb);

        TextureHandle White() const { return m_White; }
        TextureHandle Black() const { return m_Black; }
        TextureHandle Normal() const { return m_Normal; }
        TextureHandle Error() const { return m_Error; }
        SamplerHandle DefaultSampler() const { return m_DefaultSampler; }

    private:
        TextureHandle CreateFromPixels(const void* pixels, uint64_t size, uint32_t width, uint32_t height, Format format, bool generateMips, const std::string& debugName);

    private:
        GraphicsDevice& m_Device;
        FileSystem& m_FileSystem;

        TextureHandle m_White;
        TextureHandle m_Black;
        TextureHandle m_Normal;
        TextureHandle m_Error;
        SamplerHandle m_DefaultSampler;

        std::unordered_map<std::string, TextureHandle> m_Cache;
    };
}