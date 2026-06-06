#pragma once

#include <filesystem>
#include <string>

namespace Trinity
{
    class Scene;
    class Entity;
    class AssetDatabase;

    class SceneSerializer
    {
    public:
        static bool Serialize(Scene& scene, const std::filesystem::path& path, const std::string& sceneName = "Untitled");
        static bool Deserialize(Scene& scene, AssetDatabase& assetDatabase, const std::filesystem::path& path);

        static std::string SerializeEntity(Scene& scene, Entity entity);
        static Entity DeserializeEntity(Scene& scene, AssetDatabase& assetDatabase, const std::string& data, bool preserveUUIDs);
    };
}