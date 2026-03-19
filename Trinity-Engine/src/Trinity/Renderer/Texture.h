#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
	enum class TextureFormat : uint8_t
	{
		RGBA8_SRGB = 0,
		RGBA8_UNORM,
		RGBA16F,
		R8_UNORM,
		RG8_UNORM,
		D32,
		D24S8,
		BC7_SRGB,
		BC7_UNORM,
	};

	[[nodiscard]] constexpr bool IsDepthFormat(TextureFormat format) noexcept { return format == TextureFormat::D32 || format == TextureFormat::D24S8; }
	[[nodiscard]] constexpr bool IsCompressedFormat(TextureFormat format) noexcept { return format == TextureFormat::BC7_SRGB || format == TextureFormat::BC7_UNORM; }
	[[nodiscard]] constexpr bool IsColorFormat(TextureFormat format) noexcept { return !IsDepthFormat(format); }
	[[nodiscard]] constexpr bool IsSrgbFormat(TextureFormat format) noexcept { return format == TextureFormat::RGBA8_SRGB || format == TextureFormat::BC7_SRGB; }

	class Texture2D
	{
	public:
		virtual ~Texture2D() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevels() const = 0;
		virtual TextureFormat GetFormat() const = 0;
		virtual bool IsValid() const = 0;

		virtual void* GetImageViewHandle() const = 0;
		virtual void* GetSamplerHandle() const = 0;

		const std::string& GetPath() const { return m_Path; }

	protected:
		std::string m_Path;
	};
}