#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
	enum class TextureFormat : uint8_t
	{
		None = 0,
		RGBA8,
		BGRA8,
		RGBA16F,
		RGBA32F,
		RG16F,
		RG32F,
		R8,
		R32F,
		Depth32F,
		Depth24Stencil8
	};

	enum class TextureUsage : uint32_t
	{
		None = 0,
		Sampled = 1 << 0,
		Storage = 1 << 1,
		ColorAttachment = 1 << 2,
		DepthStencilAttachment = 1 << 3,
		TransferSource = 1 << 4,
		TransferDestination = 1 << 5
	};

	inline TextureUsage operator|(TextureUsage a, TextureUsage b) { return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
	inline bool operator&(TextureUsage a, TextureUsage b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }

	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t MipLevels = 1;
		TextureFormat Format = TextureFormat::RGBA8;
		TextureUsage Usage = TextureUsage::Sampled;
		std::string DebugName;
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint64_t GetOpaqueHandle() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevels() const = 0;

		const TextureSpecification& GetSpecification() const { return m_Specification; }

	protected:
		TextureSpecification m_Specification;
	};
}