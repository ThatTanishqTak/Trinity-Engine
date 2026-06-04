#pragma once

#include <filesystem>
#include <string>

namespace Trinity
{
    class Scene;
    class MeshLibrary;

    class SceneSerializer
    {
    public:
        static bool Serialize(Scene& scene, const std::filesystem::path& path, const std::string& sceneName = "Untitled");
        static bool Deserialize(Scene& scene, MeshLibrary& meshLibrary, const std::filesystem::path& path);
    };
}