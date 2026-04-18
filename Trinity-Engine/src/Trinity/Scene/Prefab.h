#pragma once

#include <string>

namespace Trinity
{
    class Entity;
    class Scene;

    class Prefab
    {
    public:
        static void Save(Entity entity, const std::string& path);
        static Entity Instantiate(Scene& scene, const std::string& path);
    };
}
