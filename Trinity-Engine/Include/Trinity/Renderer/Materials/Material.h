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

    public:
        Material()
        {
            m_Parameters[BaseColorFactor] = MaterialParameter::MakeVec4(glm::vec4(1.0f));
            m_Parameters[BaseColorTexture] = MaterialParameter::MakeTexture(UUID(0));
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