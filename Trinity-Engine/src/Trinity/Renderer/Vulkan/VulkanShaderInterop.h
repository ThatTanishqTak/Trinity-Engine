#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Trinity
{
	enum class ColorTransferMode : uint32_t
	{
		None = 0,
		SrgbToLinear = 1,
		LinearToSrgb = 2,
	};

	struct SimplePushConstants
	{
		glm::mat4 ModelViewProjection;
		glm::vec4 Color;
		uint32_t ColorInputTransfer = static_cast<uint32_t>(ColorTransferMode::None);
		uint32_t ColorOutputTransfer = static_cast<uint32_t>(ColorTransferMode::None);
	};

	static_assert(sizeof(SimplePushConstants) % 4 == 0);
	static_assert(sizeof(SimplePushConstants) <= 128);

	struct GeometryBufferPushConstants
	{
		glm::mat4 ModelViewProjection;
		glm::mat4 Model;
		glm::vec4 Color;
	};

	static_assert(sizeof(GeometryBufferPushConstants) % 4 == 0);
	static_assert(sizeof(GeometryBufferPushConstants) <= 144);

	struct ShadowPushConstants
	{
		glm::mat4 LightSpaceModelViewProjection;
	};

	static_assert(sizeof(ShadowPushConstants) % 4 == 0);
	static_assert(sizeof(ShadowPushConstants) <= 128);

	struct LightingPushConstants
	{
		glm::mat4 InvViewProjection;
		glm::vec4 CameraPosition;
		uint32_t  ColorOutputTransfer = static_cast<uint32_t>(ColorTransferMode::None);
		float CameraNear = 0.01f;
		float CameraFar = 1000.0f;
		float Exposure = 1.0f;
	};

	static_assert(sizeof(LightingPushConstants) % 4 == 0);
	static_assert(sizeof(LightingPushConstants) <= 128);

	static constexpr uint32_t MaxLights = 32;

	struct alignas(16) LightData
	{
		glm::vec4 PositionAndType;
		glm::vec4 DirectionAndIntensity;
		glm::vec4 ColorAndRange;
		glm::vec4 SpotAngles;
	};

	struct LightingUniformData
	{
		LightData Lights[MaxLights];
		glm::mat4 DirectionalLightSpaceMatrix;
		uint32_t LightCount = 0;
		uint32_t ShadowsEnabled = 0;
		uint32_t _Pad[2] = {};
	};

	// Lighting pass set 0: GBuffer + depth + shadow inputs
	static constexpr uint32_t LightingBinding_Albedo = 0; // sampler2D u_Albedo
	static constexpr uint32_t LightingBinding_Normal = 1; // sampler2D u_Normal
	static constexpr uint32_t LightingBinding_Material = 2; // sampler2D u_Material
	static constexpr uint32_t LightingBinding_Depth = 3; // sampler2D u_Depth
	static constexpr uint32_t LightingBinding_ShadowMap = 4; // sampler2DShadow u_ShadowMap

	// Lighting pass set 1: uniform buffer
	static constexpr uint32_t LightingBinding_LightUBO = 0; // LightingUniforms

	// GBuffer pass set 0: albedo texture input
	static constexpr uint32_t GBufferBinding_AlbedoTex = 0; // sampler2D u_AlbedoTexture
}