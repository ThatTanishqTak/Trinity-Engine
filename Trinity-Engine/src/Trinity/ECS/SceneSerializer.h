#pragma once

#include <memory>
#include <string>

namespace Trinity
{
    class Scene;

    class SceneSerializer
    {
    public:
        explicit SceneSerializer(Scene& scene);

        void Serialize(const std::string& filePath);
        bool Deserialize(const std::string& filePath);

    private:
        Scene& m_Scene;
    };
}