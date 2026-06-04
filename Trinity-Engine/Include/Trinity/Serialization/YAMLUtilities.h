#pragma once

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

namespace YAML
{
    template<>
    struct convert<glm::vec2>
    {
        static Node encode(const glm::vec2& vector)
        {
            Node l_Node;
            l_Node.push_back(vector.x);
            l_Node.push_back(vector.y);
            l_Node.SetStyle(EmitterStyle::Flow);

            return l_Node;
        }

        static bool decode(const Node& node, glm::vec2& vector)
        {
            if (!node.IsSequence() || node.size() != 2)
            {
                return false;
            }

            vector.x = node[0].as<float>();
            vector.y = node[1].as<float>();

            return true;
        }
    };

    template<>
    struct convert<glm::vec3>
    {
        static Node encode(const glm::vec3& vector)
        {
            Node l_Node;
            l_Node.push_back(vector.x);
            l_Node.push_back(vector.y);
            l_Node.push_back(vector.z);
            l_Node.SetStyle(EmitterStyle::Flow);

            return l_Node;
        }

        static bool decode(const Node& node, glm::vec3& vector)
        {
            if (!node.IsSequence() || node.size() != 3)
            {
                return false;
            }

            vector.x = node[0].as<float>();
            vector.y = node[1].as<float>();
            vector.z = node[2].as<float>();

            return true;
        }
    };

    template<>
    struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& vector)
        {
            Node l_Node;
            l_Node.push_back(vector.x);
            l_Node.push_back(vector.y);
            l_Node.push_back(vector.z);
            l_Node.push_back(vector.w);
            l_Node.SetStyle(EmitterStyle::Flow);

            return l_Node;
        }

        static bool decode(const Node& node, glm::vec4& vector)
        {
            if (!node.IsSequence() || node.size() != 4)
            {
                return false;
            }

            vector.x = node[0].as<float>();
            vector.y = node[1].as<float>();
            vector.z = node[2].as<float>();
            vector.w = node[3].as<float>();

            return true;
        }
    };
}

namespace Trinity
{
    inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& vector)
    {
        out << YAML::Flow << YAML::BeginSeq << vector.x << vector.y << YAML::EndSeq;

        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vector)
    {
        out << YAML::Flow << YAML::BeginSeq << vector.x << vector.y << vector.z << YAML::EndSeq;

        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& vector)
    {
        out << YAML::Flow << YAML::BeginSeq << vector.x << vector.y << vector.z << vector.w << YAML::EndSeq;

        return out;
    }
}