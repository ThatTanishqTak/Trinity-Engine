#pragma once

#include "Trinity/Geometry/Vertex.h"
#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanCommand;
	class VulkanDevice;

	namespace Geometry
	{
		struct Mesh
		{
			VulkanBuffer VertexBuffer{};
			VulkanBuffer IndexBuffer{};
			uint32_t IndexCount = 0;

			void Upload(const VulkanDevice& deviceRef, const VulkanCommand& command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

			void Destroy();
		};
	}
}