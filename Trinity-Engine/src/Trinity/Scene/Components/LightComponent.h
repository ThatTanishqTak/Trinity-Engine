#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <variant>

namespace Trinity
{
    enum class LightType : uint8_t
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
        Rect = 3,
        Capsule = 4,
        Sphere = 5
    };

    struct DirectionalLight
    {
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
    };

    struct PointLight
    {
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Range = 10.0f;
    };

    struct SpotLight
    {
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Range = 10.0f;
        float InnerConeAngle = 0.5f;
        float OuterConeAngle = 0.5f;
    };

    struct RectLight
    {
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Width = 1.0f;
        float Height = 1.0f;
    };

    struct CapsuleLight
    {
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Length = 1.0f;
        float Radius = 0.1f;
    };

    struct SphereLight
    {
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Radius = 0.5f;
    };

    using LightData = std::variant<DirectionalLight, PointLight, SpotLight, RectLight, CapsuleLight, SphereLight>;

    struct LightComponent
    {
        LightData Data = DirectionalLight{};
    };

    inline LightType GetLightType(const LightComponent& light)
    {
        return static_cast<LightType>(light.Data.index());
    }

    inline const char* GetLightTypeName(LightType type)
    {
        switch (type)
        {
            case LightType::Directional:
            {
                return "Directional";
            }
            case LightType::Point:
            {
                return "Point";
            }
            case LightType::Spot:
            {
                return "Spot";
            }
            case LightType::Rect:
            {
                return "Rect";
            }
            case LightType::Capsule:
            {
                return "Capsule";
            }
            case LightType::Sphere:
            {
                return "Sphere";
            }
        }

        return "Unknown";
    }

    inline void SetLightType(LightComponent& light, LightType type)
    {
        glm::vec3 l_Color = { 1.0f, 1.0f, 1.0f };
        float l_Intensity = 1.0f;

        std::visit([&](const auto& current)
        {
            l_Color = current.Color;
            l_Intensity = current.Intensity;
        }, light.Data);

        switch (type)
        {
            case LightType::Directional:
            {
                light.Data = DirectionalLight{ l_Color, l_Intensity };
                break;
            }
            case LightType::Point:
            {
                light.Data = PointLight{ l_Color, l_Intensity, 10.0f };
                break;
            }
            case LightType::Spot:
            {
                light.Data = SpotLight{ l_Color, l_Intensity, 10.0f, 0.5f, 0.5f };
                break;
            }
            case LightType::Rect:
            {
                light.Data = RectLight{ l_Color, l_Intensity, 1.0f, 1.0f };
                break;
            }
            case LightType::Capsule:
            {
                light.Data = CapsuleLight{ l_Color, l_Intensity, 1.0f, 0.1f };
                break;
            }
            case LightType::Sphere:
            {
                light.Data = SphereLight{ l_Color, l_Intensity, 0.5f };
                break;
            }
        }
    }
}