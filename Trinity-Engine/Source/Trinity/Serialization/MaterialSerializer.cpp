#include <Trinity/Serialization/MaterialSerializer.h>

#include <fstream>
#include <system_error>
#include <string>
#include <utility>

#include <Trinity/Serialization/YAMLUtilities.h>

#include <Trinity/Renderer/Materials/Material.h>
#include <Trinity/Renderer/Materials/MaterialInstance.h>
#include <Trinity/Renderer/Materials/MaterialParameter.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static constexpr uint32_t k_MaterialVersion = 1;

    static void EmitParameter(YAML::Emitter& out, const std::string& name, const MaterialParameter& parameter)
    {
        out << YAML::Key << name << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Type" << YAML::Value << MaterialParameterTypeToString(parameter.Type);
        out << YAML::Key << "Value" << YAML::Value;

        switch (parameter.Type)
        {
            case MaterialParameterType::Float: 
                out << parameter.AsFloat(); 
                break;
            case MaterialParameterType::Vec2: 
                out << parameter.AsVec2(); 
                break;
            case MaterialParameterType::Vec3: 
                out << parameter.AsVec3(); 
                break;
            case MaterialParameterType::Vec4: 
                out << parameter.AsVec4(); 
                break;
            case MaterialParameterType::Texture: 
                out << static_cast<uint64_t>(parameter.AsTexture()); 
                break;
            default: 
                out << 0; 
                break;
        }

        out << YAML::EndMap;
    }

    static MaterialParameter ReadParameter(const YAML::Node& node)
    {
        if (!node["Type"] || !node["Value"])
        {
            return MaterialParameter();
        }

        MaterialParameterType l_Type = MaterialParameterTypeFromString(node["Type"].as<std::string>());
        const YAML::Node& l_Value = node["Value"];

        switch (l_Type)
        {
            case MaterialParameterType::Float: 
                return MaterialParameter::MakeFloat(l_Value.as<float>());
            case MaterialParameterType::Vec2: 
                return MaterialParameter::MakeVec2(l_Value.as<glm::vec2>());
            case MaterialParameterType::Vec3: 
                return MaterialParameter::MakeVec3(l_Value.as<glm::vec3>());
            case MaterialParameterType::Vec4: 
                return MaterialParameter::MakeVec4(l_Value.as<glm::vec4>());
            case MaterialParameterType::Texture: 
                return MaterialParameter::MakeTexture(UUID(l_Value.as<uint64_t>()));
            default: 
                return MaterialParameter();
        }
    }

    static bool WriteDocument(const YAML::Emitter& out, const std::filesystem::path& path)
    {
        std::error_code l_DirectoryError;
        std::filesystem::create_directories(path.parent_path(), l_DirectoryError);

        std::ofstream l_Stream(path);
        if (!l_Stream.is_open())
        {
            TR_CORE_ERROR("MaterialSerializer: cannot open '{}' for writing", path.string());

            return false;
        }

        l_Stream << out.c_str();

        return true;
    }

    bool MaterialSerializer::SerializeMaterial(const Material& material, const std::filesystem::path& path)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Version" << YAML::Value << k_MaterialVersion;
        l_Out << YAML::Key << "Parameters" << YAML::Value << YAML::BeginMap;

        for (const std::pair<const std::string, MaterialParameter>& it_Parameter : material.GetParameters())
        {
            EmitParameter(l_Out, it_Parameter.first, it_Parameter.second);
        }

        l_Out << YAML::EndMap;
        l_Out << YAML::EndMap;

        if (!WriteDocument(l_Out, path))
        {
            return false;
        }

        TR_CORE_INFO("MaterialSerializer: saved material to '{}'", path.string());

        return true;
    }

    std::optional<Material> MaterialSerializer::DeserializeMaterial(const std::filesystem::path& path)
    {
        try
        {
            YAML::Node l_Root = YAML::LoadFile(path.string());

            Material l_Material;
            if (YAML::Node l_Parameters = l_Root["Parameters"])
            {
                for (YAML::const_iterator it_Parameter = l_Parameters.begin(); it_Parameter != l_Parameters.end(); ++it_Parameter)
                {
                    l_Material.SetParameter(it_Parameter->first.as<std::string>(), ReadParameter(it_Parameter->second));
                }
            }

            return l_Material;
        }
        catch (const std::exception& a_Exception)
        {
            TR_CORE_ERROR("MaterialSerializer: failed to load material '{}' ({})", path.string(), a_Exception.what());

            return std::nullopt;
        }
    }

    bool MaterialSerializer::SerializeMaterialInstance(const MaterialInstance& instance, const std::filesystem::path& path)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Version" << YAML::Value << k_MaterialVersion;
        l_Out << YAML::Key << "Parent" << YAML::Value << static_cast<uint64_t>(instance.GetParent());
        l_Out << YAML::Key << "Overrides" << YAML::Value << YAML::BeginMap;

        for (const std::pair<const std::string, MaterialParameter>& it_Override : instance.GetOverrides())
        {
            EmitParameter(l_Out, it_Override.first, it_Override.second);
        }

        l_Out << YAML::EndMap;
        l_Out << YAML::EndMap;

        if (!WriteDocument(l_Out, path))
        {
            return false;
        }

        TR_CORE_INFO("MaterialSerializer: saved material instance to '{}'", path.string());

        return true;
    }

    std::optional<MaterialInstance> MaterialSerializer::DeserializeMaterialInstance(const std::filesystem::path& path)
    {
        try
        {
            YAML::Node l_Root = YAML::LoadFile(path.string());

            MaterialInstance l_Instance;
            if (l_Root["Parent"])
            {
                l_Instance.SetParent(UUID(l_Root["Parent"].as<uint64_t>()));
            }

            if (YAML::Node l_Overrides = l_Root["Overrides"])
            {
                for (YAML::const_iterator it_Override = l_Overrides.begin(); it_Override != l_Overrides.end(); ++it_Override)
                {
                    l_Instance.SetOverride(it_Override->first.as<std::string>(), ReadParameter(it_Override->second));
                }
            }

            return l_Instance;
        }
        catch (const std::exception& a_Exception)
        {
            TR_CORE_ERROR("MaterialSerializer: failed to load material instance '{}' ({})", path.string(), a_Exception.what());

            return std::nullopt;
        }
    }
}