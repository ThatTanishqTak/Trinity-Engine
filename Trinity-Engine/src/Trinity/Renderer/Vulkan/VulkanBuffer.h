#pragma once

#include "Trinity/Renderer/Buffer.h"

#include <vulkan/vulkan.h>
#include <cstdint>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanBuffer
	{
	public:
		VulkanBuffer() = default;
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		VulkanBuffer(VulkanBuffer&& other) noexcept;
		VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

		void Create(const VulkanContext& context, const VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, BufferMemoryUsage memoryUsage);
		void Destroy();

		void SetData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

		void* Map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
		void Unmap();

		VkBuffer GetBuffer() const { return m_Buffer; }
		VkDeviceSize GetSize() const { return m_Size; }
		BufferMemoryUsage GetMemoryUsage() const { return m_MemoryUsage; }

	private:
		void CreateRawBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		void CopyBufferImmediate(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize dstOffset = 0, VkDeviceSize srcOffset = 0);

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_Queue = VK_NULL_HANDLE;
		uint32_t m_QueueFamilyIndex = 0;

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkDeviceSize m_Size = 0;
		BufferMemoryUsage m_MemoryUsage = BufferMemoryUsage::GPUOnly;
		void* m_Mapped = nullptr;
	};

	class VulkanVertexBuffer final : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const VulkanContext& context, const VulkanDevice& device, uint64_t size, uint32_t stride,
			BufferMemoryUsage memoryUsage = BufferMemoryUsage::GPUOnly, const void* initialData = nullptr);
		~VulkanVertexBuffer() override;

		void SetData(const void* data, uint64_t size, uint64_t offset = 0) override;

		uint64_t GetSize() const override { return (uint64_t)m_Buffer.GetSize(); }
		uint32_t GetStride() const override { return m_Stride; }
		uint64_t GetNativeHandle() const override { return (uint64_t)(uintptr_t)m_Buffer.GetBuffer(); }

		VkBuffer GetVkBuffer() const { return m_Buffer.GetBuffer(); }

	private:
		VulkanBuffer m_Buffer{};
		uint32_t m_Stride = 0;
	};

	class VulkanIndexBuffer final : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const VulkanContext& context, const VulkanDevice& device, uint64_t size, uint32_t indexCount,
			IndexType indexType = IndexType::UInt32, BufferMemoryUsage memoryUsage = BufferMemoryUsage::GPUOnly, const void* initialData = nullptr);
		~VulkanIndexBuffer() override;

		void SetData(const void* data, uint64_t size, uint64_t offset = 0) override;

		uint64_t GetSize() const override { return (uint64_t)m_Buffer.GetSize(); }
		uint32_t GetIndexCount() const override { return m_IndexCount; }
		IndexType GetIndexType() const override { return m_IndexType; }
		uint64_t GetNativeHandle() const override { return (uint64_t)(uintptr_t)m_Buffer.GetBuffer(); }

		VkBuffer GetVkBuffer() const { return m_Buffer.GetBuffer(); }

	private:
		VulkanBuffer m_Buffer{};
		uint32_t m_IndexCount = 0;
		IndexType m_IndexType = IndexType::UInt32;
	};

	inline VkIndexType ToVkIndexType(IndexType type)
	{
		return (type == IndexType::UInt16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
	}
}