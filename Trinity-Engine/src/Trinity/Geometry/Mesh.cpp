#include "Trinity/Geometry/Mesh.h"

#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include <cstring>

namespace Trinity
{
	namespace Geometry
	{
		void Mesh::Upload(const VulkanDevice& deviceRef, const VulkanCommand& command, VkQueue queue, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
		{
			IndexCount = static_cast<uint32_t>(indices.size());

			if (vertices.empty() || indices.empty())
			{
				VertexBuffer.Destroy();
				IndexBuffer.Destroy();
				return;
			}

			VulkanBuffer l_VertexStagingBuffer{};
			VulkanBuffer l_IndexStagingBuffer{};

			VkDeviceSize l_VertexSize = static_cast<VkDeviceSize>(vertices.size() * sizeof(Vertex));
			VkDeviceSize l_IndexSize = static_cast<VkDeviceSize>(indices.size() * sizeof(uint32_t));

			l_VertexStagingBuffer.Create(deviceRef, l_VertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			void* l_VertexData = l_VertexStagingBuffer.Map();
			std::memcpy(l_VertexData, vertices.data(), static_cast<size_t>(l_VertexSize));
			l_VertexStagingBuffer.Unmap();

			l_IndexStagingBuffer.Create(deviceRef, l_IndexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			void* l_IndexData = l_IndexStagingBuffer.Map();
			std::memcpy(l_IndexData, indices.data(), static_cast<size_t>(l_IndexSize));
			l_IndexStagingBuffer.Unmap();

			VertexBuffer.Destroy();
			IndexBuffer.Destroy();

			VertexBuffer.Create(deviceRef, l_VertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			IndexBuffer.Create(deviceRef, l_IndexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VertexBuffer.CopyFromStaging(command, l_VertexStagingBuffer, queue);
			IndexBuffer.CopyFromStaging(command, l_IndexStagingBuffer, queue);

			l_VertexStagingBuffer.Destroy();
			l_IndexStagingBuffer.Destroy();
		}

		void Mesh::Destroy()
		{
			VertexBuffer.Destroy();
			IndexBuffer.Destroy();
			IndexCount = 0;
		}
	}
}