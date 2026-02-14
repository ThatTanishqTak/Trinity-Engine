#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Trinity
{
	enum class ColorTransferMode : uint32_t
	{
		None = 0,
		SrgbToLinear = 1,
		LinearToSrgb = 2
	};

	struct SimplePushConstants
	{
		glm::mat4 ModelViewProjection;
		glm::vec4 Color;
		uint32_t ColorInputTransfer = static_cast<uint32_t>(ColorTransferMode::None);
		uint32_t ColorOutputTransfer = static_cast<uint32_t>(ColorTransferMode::None);
	};

	static_assert(sizeof(SimplePushConstants) % 4 == 0, "Push constant size must be a multiple of 4 bytes");
	static_assert(sizeof(SimplePushConstants) <= 128, "Push constants exceed a conservative 128-byte budget");
}