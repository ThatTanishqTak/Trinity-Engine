#pragma once

#include <cstdint>

namespace Trinity
{
	enum class BufferMemoryUsage : uint8_t
	{
		GPUOnly = 0,  // Device local, staging upload
		CPUToGPU,     // Host visible/coherent
	};

	enum class IndexType : uint8_t
	{
		UInt16 = 0,
		UInt32,
	};

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void SetData(const void* data, uint64_t size, uint64_t offset = 0) = 0;

		virtual uint64_t GetSize() const = 0;
		virtual uint32_t GetStride() const = 0;

		virtual uint64_t GetNativeHandle() const = 0;
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void SetData(const void* data, uint64_t size, uint64_t offset = 0) = 0;

		virtual uint64_t GetSize() const = 0;
		virtual uint32_t GetIndexCount() const = 0;
		virtual IndexType GetIndexType() const = 0;

		virtual uint64_t GetNativeHandle() const = 0;
	};

	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() = default;
		virtual void SetData(const void* data, uint64_t size, uint64_t offset = 0) = 0;

		template<typename T>
		void SetData(const T& value, uint64_t offset = 0)
		{
			SetData(&value, sizeof(T), offset);
		}

		virtual uint64_t GetSize() const = 0;
		virtual uint64_t GetNativeHandle() const = 0;
	};
}