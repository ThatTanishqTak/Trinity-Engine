#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Trinity
{
	enum class SceneColorOutputTransfer : uint32_t
	{
		None = 0,
		LinearToSrgb = 1
	};

	struct SimplePushConstants
	{
		glm::mat4 ModelViewProjection;
		glm::vec4 Color;
		uint32_t ColorOutputTransfer = static_cast<uint32_t>(SceneColorOutputTransfer::None);
	};

	static_assert(sizeof(SimplePushConstants) % 4 == 0, "Push constant size must be a multiple of 4 bytes");
	static_assert(sizeof(SimplePushConstants) <= 128, "Push constants exceed a conservative 128-byte budget");
}