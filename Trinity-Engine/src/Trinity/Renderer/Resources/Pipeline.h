#pragma once

#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
	enum class PrimitiveTopology : uint8_t
	{
		TriangleList = 0,
		TriangleStrip,
		LineList,
		LineStrip,
		PointList
	};

	enum class CullMode : uint8_t
	{
		None = 0,
		Front,
		Back,
		FrontAndBack
	};

	enum class DepthCompareOp : uint8_t
	{
		Never = 0,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always
	};

	enum class VertexAttributeFormat : uint8_t
	{
		Float = 0,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		UInt,
		UInt2,
		UInt3,
		UInt4
	};

	struct VertexAttribute
	{
		uint32_t Location = 0;
		uint32_t Binding = 0;
		VertexAttributeFormat Format = VertexAttributeFormat::Float3;
		uint32_t Offset = 0;
	};

	struct PushConstantRange
	{
		ShaderStage Stage = ShaderStage::Vertex;
		uint32_t Offset = 0;
		uint32_t Size = 0;
	};

	enum class DescriptorType : uint8_t
	{
		CombinedImageSampler = 0,
		UniformBuffer,
		StorageBuffer,
	};

	struct DescriptorSetLayoutBinding
	{
		uint32_t Binding = 0;
		DescriptorType Type = DescriptorType::CombinedImageSampler;
		ShaderStage Stage = ShaderStage::Fragment;
		uint32_t Count = 1;
	};

	struct PipelineSpecification
	{
		std::shared_ptr<Shader> PipelineShader;
		std::vector<VertexAttribute> VertexAttributes;
		uint32_t VertexStride = 0;
		std::vector<PushConstantRange> PushConstants;

		PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
		CullMode CullingMode = CullMode::Back;
		bool DepthTest = true;
		bool DepthWrite = true;
		DepthCompareOp DepthOp = DepthCompareOp::Less;
		bool WireframeMode = false;
		bool DepthBias = false;
		float DepthBiasConstantFactor = 0.0f;
		float DepthBiasSlopeFactor = 0.0f;

		std::vector<DescriptorSetLayoutBinding> DescriptorBindings;

		std::vector<TextureFormat> ColorAttachmentFormats;
		TextureFormat DepthAttachmentFormat = TextureFormat::None;

		std::string DebugName;
	};

	class Pipeline
	{
	public:
		virtual ~Pipeline() = default;

		const PipelineSpecification& GetSpecification() const { return m_Specification; }

	protected:
		PipelineSpecification m_Specification;
	};
}