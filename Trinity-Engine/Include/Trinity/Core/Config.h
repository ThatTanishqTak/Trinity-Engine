#pragma once

#include <string>
#include <filesystem>

#include <yaml-cpp/yaml.h>

namespace Trinity
{
    class Config
    {
    public:
        Config() = default;

        bool Load(const std::filesystem::path& path);
        bool Save(const std::filesystem::path& path) const;

        bool Has(const std::string& key) const;

        template<typename T>
        T Get(const std::string& key, const T& defaultType = T()) const
        {
            const YAML::Node l_Node = Resolve(key);
            if (!l_Node)
            {
                return defaultType;
            }

            try
            {
                return l_Node.as<T>();
            }
            catch (const YAML::Exception&)
            {
                return defaultType;
            }
        }

        template<typename T>
        void Set(const std::string& key, const T& value)
        {
            ResolveOrCreate(key) = value;
        }

        YAML::Node& Root() { return m_Root; }
        const YAML::Node& Root() const { return m_Root; }

    private:
        YAML::Node Resolve(const std::string& key) const;
        YAML::Node ResolveOrCreate(const std::string& key);

        YAML::Node m_Root;
    };
}