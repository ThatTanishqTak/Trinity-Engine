#pragma once

#include "Trinity/Asset/AssetHandle.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace Trinity
{
    enum class MaterialAlphaMode : uint8_t
    {
        Opaque = 0,
        Masked,
        Blend
    };

    struct MaterialTextureSlot
    {
        AssetHandle TextureAssetUUID = InvalidAsset;
        std::shared_ptr<Texture> TextureData;
        bool Enabled = false;

        bool IsValid() const { return Enabled && TextureAssetUUID != InvalidAsset && TextureData != nullptr; }

        void Clear()
        {
            TextureAssetUUID = InvalidAsset;
            TextureData.reset();
            Enabled = false;
        }
    };

    struct MaterialProperties
    {
        glm::vec4 BaseColor = glm::vec4(1.0f);

        float Metallic = 0.0f;
        float Roughness = 0.5f;
        float AmbientOcclusion = 1.0f;

        glm::vec3 EmissiveColor = glm::vec3(0.0f);
        float EmissiveStrength = 0.0f;

        float AlphaCutoff = 0.5f;
        MaterialAlphaMode AlphaMode = MaterialAlphaMode::Opaque;

        MaterialTextureSlot AlbedoTexture;
        MaterialTextureSlot NormalTexture;
        MaterialTextureSlot MetallicRoughnessTexture;
        MaterialTextureSlot AmbientOcclusionTexture;
        MaterialTextureSlot EmissiveTexture;
    };

    class Material
    {
    public:
        Material() = default;
        explicit Material(std::string name) : m_Name(std::move(name))
        {

        }

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        MaterialProperties& GetProperties() { return m_Properties; }
        const MaterialProperties& GetProperties() const { return m_Properties; }

        void SetBaseColor(const glm::vec4& color) { m_Properties.BaseColor = color; }
        const glm::vec4& GetBaseColor() const { return m_Properties.BaseColor; }

        void SetMetallic(float metallic) { m_Properties.Metallic = metallic; }
        float GetMetallic() const { return m_Properties.Metallic; }

        void SetRoughness(float roughness) { m_Properties.Roughness = roughness; }
        float GetRoughness() const { return m_Properties.Roughness; }

        void SetAmbientOcclusion(float ambientOcclusion) { m_Properties.AmbientOcclusion = ambientOcclusion; }
        float GetAmbientOcclusion() const { return m_Properties.AmbientOcclusion; }

        void SetEmissiveColor(const glm::vec3& color) { m_Properties.EmissiveColor = color; }
        const glm::vec3& GetEmissiveColor() const { return m_Properties.EmissiveColor; }

        void SetEmissiveStrength(float strength) { m_Properties.EmissiveStrength = strength; }
        float GetEmissiveStrength() const { return m_Properties.EmissiveStrength; }

    private:
        std::string m_Name = "Material";
        MaterialProperties m_Properties;
    };
}