#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
	enum class TextureFormat : uint8_t
	{
		RGBA8_SRGB = 0,
		RGBA8_UNORM,
	};

	class Texture2D
	{
	public:
		virtual ~Texture2D() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevels() const = 0;
		virtual bool IsValid() const = 0;

		virtual void* GetImageViewHandle() const = 0;
		virtual void* GetSamplerHandle() const = 0;

		const std::string& GetPath() const { return m_Path; }

	protected:
		std::string m_Path;
	};
}