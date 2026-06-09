#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include <Trinity/Core/UUID.h>

namespace Trinity
{
    enum class MaterialParameterType : uint32_t
    {
        None = 0,
        Float,
        Vec2,
        Vec3,
        Vec4,
        Texture
    };

    inline const char* MaterialParameterTypeToString(MaterialParameterType type)
    {
        switch (type)
        {
            case MaterialParameterType::Float:
                return "Float";
            case MaterialParameterType::Vec2:
                return "Vec2";
            case MaterialParameterType::Vec3:
                return "Vec3";
            case MaterialParameterType::Vec4:
                return "Vec4";
            case MaterialParameterType::Texture:
                return "Texture";
            default:
                return "None";
        }
    }

    inline MaterialParameterType MaterialParameterTypeFromString(const std::string& text)
    {
        if (text == "Float")
        {
            return MaterialParameterType::Float;
        }

        if (text == "Vec2")
        {
            return MaterialParameterType::Vec2;
        }
        
        if (text == "Vec3")
        {
            return MaterialParameterType::Vec3;
        }
        
        if (text == "Vec4")
        {
            return MaterialParameterType::Vec4;
        }
     
        if (text == "Texture")
        {
            return MaterialParameterType::Texture;
        }

        return MaterialParameterType::None;
    }

    struct MaterialParameter
    {
        MaterialParameterType Type = MaterialParameterType::None;
        glm::vec4 Value = glm::vec4(0.0f);
        UUID Texture = UUID(0);

        static MaterialParameter MakeFloat(float value)
        {
            MaterialParameter l_Parameter;
            l_Parameter.Type = MaterialParameterType::Float;
            l_Parameter.Value = glm::vec4(value, 0.0f, 0.0f, 0.0f);

            return l_Parameter;
        }

        static MaterialParameter MakeVec2(const glm::vec2& value)
        {
            MaterialParameter l_Parameter;
            l_Parameter.Type = MaterialParameterType::Vec2;
            l_Parameter.Value = glm::vec4(value, 0.0f, 0.0f);

            return l_Parameter;
        }

        static MaterialParameter MakeVec3(const glm::vec3& value)
        {
            MaterialParameter l_Parameter;
            l_Parameter.Type = MaterialParameterType::Vec3;
            l_Parameter.Value = glm::vec4(value, 0.0f);

            return l_Parameter;
        }

        static MaterialParameter MakeVec4(const glm::vec4& value)
        {
            MaterialParameter l_Parameter;
            l_Parameter.Type = MaterialParameterType::Vec4;
            l_Parameter.Value = value;

            return l_Parameter;
        }

        static MaterialParameter MakeTexture(UUID texture)
        {
            MaterialParameter l_Parameter;
            l_Parameter.Type = MaterialParameterType::Texture;
            l_Parameter.Texture = texture;

            return l_Parameter;
        }

        float AsFloat() const { return Value.x; }
        glm::vec2 AsVec2() const { return glm::vec2(Value); }
        glm::vec3 AsVec3() const { return glm::vec3(Value); }
        glm::vec4 AsVec4() const { return Value; }
        UUID AsTexture() const { return Texture; }
    };
}