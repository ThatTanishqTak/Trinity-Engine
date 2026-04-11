#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Trinity
{
	enum class ShaderStage : uint8_t
	{
		None = 0,
		Vertex,
		Fragment,
		Compute,
		Geometry
	};

	struct ShaderModuleSpecification
	{
		ShaderStage Stage = ShaderStage::None;
		std::string SpvPath;
	};

	struct ShaderSpecification
	{
		std::vector<ShaderModuleSpecification> Modules;
		std::string DebugName;
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;

		const ShaderSpecification& GetSpecification() const { return m_Specification; }

	protected:
		ShaderSpecification m_Specification;
	};
}