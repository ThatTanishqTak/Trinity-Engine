#pragma once

#include <string>

namespace Trinity
{
    class Scene;

    class SceneSerializer
    {
    public:
        static void Serialize(Scene& scene, const std::string& filepath);
        static std::string SerializeToString(Scene& scene);

        static bool Deserialize(Scene& scene, const std::string& filepath);
        static bool DeserializeFromString(Scene& scene, const std::string& yaml);
    };
}
