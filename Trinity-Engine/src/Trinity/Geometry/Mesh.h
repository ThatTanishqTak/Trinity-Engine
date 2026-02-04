#pragma once

#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanCommand;
	class VulkanDevice;

	namespace Geometry
	{
		struct Vertex
		{
			glm::vec3 Position{};
			glm::vec3 Normal{};
			glm::vec2 UV{};
			glm::vec4 Tangent{};
		};

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