#include "Trinity/Renderer/Material.h"

#include "Trinity/Renderer/ShaderLibrary.h"
#include "Trinity/Renderer/Shader.h"

#include "Trinity/Utilities/Log.h"

#include <cstring>
#include <algorithm>

namespace Trinity
{
	Material::Material(const std::string& name) : m_Name(name)
	{

	}

	Material::Material(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader) : m_Name(name), m_VertexShader(vertexShader), m_FragmentShader(fragmentShader)
	{

	}

	void Material::SetVertexShader(const std::string& shaderName)
	{
		m_VertexShader = shaderName;
	}

	void Material::SetFragmentShader(const std::string& shaderName)
	{
		m_FragmentShader = shaderName;
	}

	void Material::SetFloat(const std::string& name, float value)
	{
		m_Properties[name] = { MaterialPropertyType::Float, value };
	}

	void Material::SetInt(const std::string& name, int32_t value)
	{
		m_Properties[name] = { MaterialPropertyType::Int, value };
	}

	void Material::SetUInt(const std::string& name, uint32_t value)
	{
		m_Properties[name] = { MaterialPropertyType::UInt, value };
	}

	void Material::SetVec2(const std::string& name, const glm::vec2& value)
	{
		m_Properties[name] = { MaterialPropertyType::Vec2, value };
	}

	void Material::SetVec3(const std::string& name, const glm::vec3& value)
	{
		m_Properties[name] = { MaterialPropertyType::Vec3, value };
	}

	void Material::SetVec4(const std::string& name, const glm::vec4& value)
	{
		m_Properties[name] = { MaterialPropertyType::Vec4, value };
	}

	void Material::SetMat4(const std::string& name, const glm::mat4& value)
	{
		m_Properties[name] = { MaterialPropertyType::Mat4, value };
	}

	bool Material::GetFloat(const std::string& name, float& outValue) const
	{
		const auto a_It = m_Properties.find(name);
		if (a_It == m_Properties.end() || a_It->second.m_Type != MaterialPropertyType::Float)
		{
			return false;
		}
		
		outValue = std::get<float>(a_It->second.m_Value);
		
		return true;
	}

	bool Material::GetInt(const std::string& name, int32_t& outValue) const
	{
		const auto a_It = m_Properties.find(name);
		if (a_It == m_Properties.end() || a_It->second.m_Type != MaterialPropertyType::Int)
		{
			return false;
		}
		
		outValue = std::get<int32_t>(a_It->second.m_Value);
		
		return true;
	}

	bool Material::GetUInt(const std::string& name, uint32_t& outValue) const
	{
		const auto a_It = m_Properties.find(name);
		if (a_It == m_Properties.end() || a_It->second.m_Type != MaterialPropertyType::UInt)
		{
			return false;
		}
		
		outValue = std::get<uint32_t>(a_It->second.m_Value);
		
		return true;
	}

	bool Material::GetVec2(const std::string& name, glm::vec2& outValue) const
	{
		const auto a_It = m_Properties.find(name);
		if (a_It == m_Properties.end() || a_It->second.m_Type != MaterialPropertyType::Vec2)
		{
			return false;
		}
		
		outValue = std::get<glm::vec2>(a_It->second.m_Value);
		
		return true;
	}

	bool Material::GetVec3(const std::string& name, glm::vec3& outValue) const
	{
		const auto a_It = m_Properties.find(name);
		if (a_It == m_Properties.end() || a_It->second.m_Type != MaterialPropertyType::Vec3)
		{
			return false;
		}
		
		outValue = std::get<glm::vec3>(a_It->second.m_Value);
		
		return true;
	}

	bool Material::GetVec4(const std::string& name, glm::vec4& outValue) const
	{
		const auto a_It = m_Properties.find(name);
		if (a_It == m_Properties.end() || a_It->second.m_Type != MaterialPropertyType::Vec4)
		{
			return false;
		}
		
		outValue = std::get<glm::vec4>(a_It->second.m_Value);
		
		return true;
	}

	bool Material::GetMat4(const std::string& name, glm::mat4& outValue) const
	{
		const auto a_It = m_Properties.find(name);
		if (a_It == m_Properties.end() || a_It->second.m_Type != MaterialPropertyType::Mat4)
		{
			return false;
		}
		
		outValue = std::get<glm::mat4>(a_It->second.m_Value);
		
		return true;
	}

	bool Material::HasProperty(const std::string& name) const
	{
		return m_Properties.count(name) > 0;
	}

	void Material::RemoveProperty(const std::string& name)
	{
		m_Properties.erase(name);
	}

	std::vector<uint8_t> Material::PackProperties() const
	{
		if (m_PackedSize == 0)
		{
			return {};
		}

		std::vector<uint8_t> l_Buffer(m_PackedSize, 0);

		for (const auto& [l_Name, l_Prop] : m_Properties)
		{
			if (l_Prop.m_PackedOffset == ~0u)
			{
				continue;
			}

			const uint32_t l_Size = PropertyTypeSize(l_Prop.m_Type);
			const uint32_t l_End = l_Prop.m_PackedOffset + l_Size;

			if (l_End > m_PackedSize)
			{
				TR_CORE_WARN("Material '{}': property '{}' at offset {} overflows packed size {}", m_Name.c_str(), l_Name.c_str(), l_Prop.m_PackedOffset, m_PackedSize);
				continue;
			}

			WritePropertyToBuffer(l_Prop, l_Buffer, l_Prop.m_PackedOffset);
		}

		return l_Buffer;
	}

	void Material::AssignPackedOffsets(const ShaderLibrary& library)
	{
		const Shader* l_VertShader = library.Get(m_VertexShader);
		const Shader* l_FragShader = library.Get(m_FragmentShader);

		uint32_t l_PushConstantSize = 0;

		if (l_VertShader && l_VertShader->m_Reflection.m_PushConstants.m_Size > 0)
		{
			l_PushConstantSize = std::max(l_PushConstantSize, l_VertShader->m_Reflection.m_PushConstants.m_Size);
		}

		if (l_FragShader && l_FragShader->m_Reflection.m_PushConstants.m_Size > 0)
		{
			l_PushConstantSize = std::max(l_PushConstantSize, l_FragShader->m_Reflection.m_PushConstants.m_Size);
		}

		m_PackedSize = l_PushConstantSize;

		uint32_t l_CurrentOffset = 0;
		for (auto& [l_Name, l_Prop] : m_Properties)
		{
			const uint32_t l_Alignment = std::min(PropertyTypeSize(l_Prop.m_Type), 16u);
			l_CurrentOffset = (l_CurrentOffset + l_Alignment - 1) & ~(l_Alignment - 1);
			l_Prop.m_PackedOffset = l_CurrentOffset;
			l_CurrentOffset += PropertyTypeSize(l_Prop.m_Type);
		}

		if (m_PackedSize == 0 && l_CurrentOffset > 0)
		{
			m_PackedSize = (l_CurrentOffset + 3) & ~3u;
		}
	}

	uint32_t Material::PropertyTypeSize(MaterialPropertyType type)
	{
		switch (type)
		{
			case MaterialPropertyType::Float: return 4;
			case MaterialPropertyType::Int:   return 4;
			case MaterialPropertyType::UInt:  return 4;
			case MaterialPropertyType::Vec2:  return 8;
			case MaterialPropertyType::Vec3:  return 12;
			case MaterialPropertyType::Vec4:  return 16;
			case MaterialPropertyType::Mat4:  return 64;
			default:                          return 0;
		}
	}

	void Material::WritePropertyToBuffer(const MaterialProperty& prop, std::vector<uint8_t>& buffer, uint32_t offset)
	{
		switch (prop.m_Type)
		{
			case MaterialPropertyType::Float:
			{
				const float l_V = std::get<float>(prop.m_Value);
				std::memcpy(buffer.data() + offset, &l_V, sizeof(float));
				
				break;
			}
			case MaterialPropertyType::Int:
			{
				const int32_t l_V = std::get<int32_t>(prop.m_Value);
				std::memcpy(buffer.data() + offset, &l_V, sizeof(int32_t));
				break;
			}
			case MaterialPropertyType::UInt:
			{
				const uint32_t l_V = std::get<uint32_t>(prop.m_Value);
				std::memcpy(buffer.data() + offset, &l_V, sizeof(uint32_t));
				
				break;
			}
			case MaterialPropertyType::Vec2:
			{
				const glm::vec2 l_V = std::get<glm::vec2>(prop.m_Value);
				std::memcpy(buffer.data() + offset, &l_V, sizeof(glm::vec2));
				
				break;
			}
			case MaterialPropertyType::Vec3:
			{
				const glm::vec3 l_V = std::get<glm::vec3>(prop.m_Value);
				std::memcpy(buffer.data() + offset, &l_V, sizeof(glm::vec3));
				
				break;
			}
			case MaterialPropertyType::Vec4:
			{
				const glm::vec4 l_V = std::get<glm::vec4>(prop.m_Value);
				std::memcpy(buffer.data() + offset, &l_V, sizeof(glm::vec4));
				
				break;
			}
			case MaterialPropertyType::Mat4:
			{
				const glm::mat4 l_V = std::get<glm::mat4>(prop.m_Value);
				std::memcpy(buffer.data() + offset, &l_V, sizeof(glm::mat4));
				
				break;
			}
			default:
				break;
		}
	}
}