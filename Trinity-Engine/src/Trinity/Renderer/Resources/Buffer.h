#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
	enum class BufferUsage : uint32_t
	{
		None = 0,
		VertexBuffer = 1 << 0,
		IndexBuffer = 1 << 1,
		UniformBuffer = 1 << 2,
		StorageBuffer = 1 << 3,
		TransferSource = 1 << 4,
		TransferDestination = 1 << 5
	};

	inline BufferUsage operator|(BufferUsage a, BufferUsage b) { return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
	inline bool operator&(BufferUsage a, BufferUsage b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }

	enum class BufferMemoryType : uint8_t
	{
		GPU = 0,
		CPUToGPU,
		GPUToCPU
	};

	struct BufferSpecification
	{
		uint64_t Size = 0;
		BufferUsage Usage = BufferUsage::None;
		BufferMemoryType MemoryType = BufferMemoryType::GPU;
		std::string DebugName;
	};

	class Buffer
	{
	public:
		virtual ~Buffer() = default;

		virtual void SetData(const void* data, uint64_t size, uint64_t offset = 0) = 0;
		virtual void* Map() = 0;
		virtual void Unmap() = 0;
		virtual uint64_t GetSize() const = 0;

		const BufferSpecification& GetSpecification() const { return m_Specification; }

	protected:
		BufferSpecification m_Specification;
	};
}