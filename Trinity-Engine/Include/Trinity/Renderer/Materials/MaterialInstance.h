#pragma once

#include <map>
#include <string>
#include <utility>

#include <Trinity/Core/UUID.h>
#include <Trinity/Renderer/Materials/Material.h>
#include <Trinity/Renderer/Materials/MaterialParameter.h>

namespace Trinity
{
    class MaterialInstance
    {
    public:
        MaterialInstance() = default;
        explicit MaterialInstance(UUID parent) : m_Parent(parent)
        {

        }

        UUID GetParent() const { return m_Parent; }
        void SetParent(UUID parent) { m_Parent = parent; }

        const std::map<std::string, MaterialParameter>& GetOverrides() const { return m_Overrides; }

        void SetOverride(const std::string& name, const MaterialParameter& parameter) { m_Overrides[name] = parameter; }
        void ClearOverride(const std::string& name) { m_Overrides.erase(name); }
        bool HasOverride(const std::string& name) const { return m_Overrides.find(name) != m_Overrides.end(); }

        void ApplyOverrides(Material& target) const
        {
            for (const std::pair<const std::string, MaterialParameter>& it_Override : m_Overrides)
            {
                target.SetParameter(it_Override.first, it_Override.second);
            }
        }

    private:
        UUID m_Parent = UUID(0);
        std::map<std::string, MaterialParameter> m_Overrides;
    };
}