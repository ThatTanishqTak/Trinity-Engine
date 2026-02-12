#pragma once

#include <glm/glm.hpp>

namespace Trinity
{
	struct SimplePushConstants
	{
		glm::mat4 ModelViewProjection;
		glm::vec4 Color;
	};

	static_assert(sizeof(SimplePushConstants) % 4 == 0, "Push constant size must be a multiple of 4 bytes");
	static_assert(sizeof(SimplePushConstants) <= 128, "Push constants exceed a conservative 128-byte budget");
}