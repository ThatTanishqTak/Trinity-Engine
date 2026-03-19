#pragma once

#include "Trinity/Assets/AssetHandle.h"
#include "Trinity/Renderer/Texture.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Trinity
{
	class ShaderLibrary;

	using MaterialPropertyValue = std::variant<float, int32_t, uint32_t, glm::vec2, glm::vec3, glm::vec4, glm::mat4, AssetUUID>;

	enum class MaterialPropertyType : uint8_t
	{
		Float = 0,
		Int,
		UInt,
		Vec2,
		Vec3,
		Vec4,
		Mat4,
		Texture,
	};

	struct MaterialProperty
	{
		MaterialPropertyType m_Type = MaterialPropertyType::Float;
		MaterialPropertyValue m_Value;
		uint32_t m_PackedOffset = ~0u;
		TextureFormat m_TextureFormatHint = TextureFormat::RGBA8_SRGB;
	};

	class Material
	{
	public:
		Material() = default;
		explicit Material(const std::string& name);
		Material(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader);

		void SetVertexShader(const std::string& shaderName);
		void SetFragmentShader(const std::string& shaderName);

		const std::string& GetName() const { return m_Name; }
		const std::string& GetVertexShader() const { return m_VertexShader; }
		const std::string& GetFragmentShader() const { return m_FragmentShader; }

		void SetFloat(const std::string& name, float value);
		void SetInt(const std::string& name, int32_t value);
		void SetUInt(const std::string& name, uint32_t value);
		void SetVec2(const std::string& name, const glm::vec2& value);
		void SetVec3(const std::string& name, const glm::vec3& value);
		void SetVec4(const std::string& name, const glm::vec4& value);
		void SetMat4(const std::string& name, const glm::mat4& value);

		void SetTexture(const std::string& name, AssetUUID uuid, TextureFormat formatHint = TextureFormat::RGBA8_SRGB);

		bool GetFloat(const std::string& name, float& outValue) const;
		bool GetInt(const std::string& name, int32_t& outValue) const;
		bool GetUInt(const std::string& name, uint32_t& outValue) const;
		bool GetVec2(const std::string& name, glm::vec2& outValue) const;
		bool GetVec3(const std::string& name, glm::vec3& outValue) const;
		bool GetVec4(const std::string& name, glm::vec4& outValue) const;
		bool GetMat4(const std::string& name, glm::mat4& outValue) const;

		bool GetTexture(const std::string& name, AssetUUID& outUUID) const;
		TextureFormat GetTextureFormatHint(const std::string& name) const;

		bool HasProperty(const std::string& name) const;
		void RemoveProperty(const std::string& name);

		std::vector<uint8_t> PackProperties() const;
		void AssignPackedOffsets(const ShaderLibrary& library);

	private:
		static uint32_t PropertyTypeSize(MaterialPropertyType type);
		static void WritePropertyToBuffer(const MaterialProperty& prop, std::vector<uint8_t>& buffer, uint32_t offset);

	private:
		std::string m_Name;
		std::string m_VertexShader;
		std::string m_FragmentShader;
		std::unordered_map<std::string, MaterialProperty> m_Properties;
		uint32_t m_PackedSize = 0;
	};
}