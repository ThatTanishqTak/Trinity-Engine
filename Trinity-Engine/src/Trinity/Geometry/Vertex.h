#pragma once

#include <glm/glm.hpp>

namespace Trinity
{
	namespace Geometry
	{
		struct Vertex
		{
			glm::vec3 Position{};
			glm::vec3 Normal{};
			glm::vec2 UV{};
			glm::vec4 Tangent{};
		};
	}
}