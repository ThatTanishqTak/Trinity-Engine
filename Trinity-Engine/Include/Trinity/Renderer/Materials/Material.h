#pragma once

#include <map>
#include <string>

#include <glm/glm.hpp>

#include <Trinity/Core/UUID.h>
#include <Trinity/Renderer/Materials/MaterialParameter.h>

namespace Trinity
{
    class Material
    {
    public:
        static constexpr const char* BaseColorFactor = "BaseColorFactor";
        static constexpr const char* BaseColorTexture = "BaseColorTexture";
        static constexpr const char* MetallicFactor = "MetallicFactor";
        static constexpr const char* RoughnessFactor = "RoughnessFactor";
        static constexpr const char* MetallicRoughnessTexture = "MetallicRoughnessTexture";
        static constexpr const char* NormalTexture = "NormalTexture";
        static constexpr const char* NormalScale = "NormalScale";
        static constexpr const char* OcclusionStrength = "OcclusionStrength";
        static constexpr const char* EmissiveFactor = "EmissiveFactor";
        static constexpr const char* EmissiveStrength = "EmissiveStrength";
        static constexpr const char* EmissiveTexture = "EmissiveTexture";

    public:
        Material()
        {
            m_Parameters[BaseColorFactor] = MaterialParameter::MakeVec4(glm::vec4(1.0f));
            m_Parameters[BaseColorTexture] = MaterialParameter::MakeTexture(UUID(0));
            m_Parameters[MetallicFactor] = MaterialParameter::MakeFloat(0.0f);
            m_Parameters[RoughnessFactor] = MaterialParameter::MakeFloat(0.5f);
            m_Parameters[MetallicRoughnessTexture] = MaterialParameter::MakeTexture(UUID(0));
            m_Parameters[NormalTexture] = MaterialParameter::MakeTexture(UUID(0));
            m_Parameters[NormalScale] = MaterialParameter::MakeFloat(1.0f);
            m_Parameters[OcclusionStrength] = MaterialParameter::MakeFloat(1.0f);
            m_Parameters[EmissiveFactor] = MaterialParameter::MakeVec3(glm::vec3(0.0f));
            m_Parameters[EmissiveStrength] = MaterialParameter::MakeFloat(1.0f);
            m_Parameters[EmissiveTexture] = MaterialParameter::MakeTexture(UUID(0));
        }

        const std::map<std::string, MaterialParameter>& GetParameters() const { return m_Parameters; }

        void SetParameter(const std::string& name, const MaterialParameter& parameter) { m_Parameters[name] = parameter; }
        bool HasParameter(const std::string& name) const { return m_Parameters.find(name) != m_Parameters.end(); }

        const MaterialParameter* FindParameter(const std::string& name) const
        {
            auto it_Found = m_Parameters.find(name);

            return it_Found != m_Parameters.end() ? &it_Found->second : nullptr;
        }

    private:
        std::map<std::string, MaterialParameter> m_Parameters;
    };
}