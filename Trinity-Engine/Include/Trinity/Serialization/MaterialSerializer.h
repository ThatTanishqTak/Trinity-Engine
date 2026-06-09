#pragma once

#include <filesystem>
#include <optional>

namespace Trinity
{
    class Material;
    class MaterialInstance;

    class MaterialSerializer
    {
    public:
        static bool SerializeMaterial(const Material& material, const std::filesystem::path& path);
        static std::optional<Material> DeserializeMaterial(const std::filesystem::path& path);

        static bool SerializeMaterialInstance(const MaterialInstance& instance, const std::filesystem::path& path);
        static std::optional<MaterialInstance> DeserializeMaterialInstance(const std::filesystem::path& path);
    };
}