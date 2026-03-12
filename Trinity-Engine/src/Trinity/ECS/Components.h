#pragma once

#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Renderer/Material.h"
#include "Trinity/Assets/AssetHandle.h"
#include "Trinity/Assets/MeshAsset.h"

#include <cstdint>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Trinity
{
    using EntityID = uint64_t;

    struct IDComponent
    {
        EntityID ID = 0;
    };

    struct TagComponent
    {
        std::string Tag;
    };

    struct TransformComponent
    {
        glm::vec3 Translation{ 0.0f };
        glm::vec3 Rotation{ 0.0f };
        glm::vec3 Scale{ 1.0f };

        glm::mat4 GetTransform() const
        {
            const glm::mat4 l_Translate = glm::translate(glm::mat4(1.0f), Translation);

            const glm::quat l_Quaternion = glm::quat(Rotation);
            const glm::mat4 l_Rotate = glm::toMat4(l_Quaternion);
            const glm::mat4 l_Scale = glm::scale(glm::mat4(1.0f), Scale);

            return l_Translate * l_Rotate * l_Scale;
        }
    };

    struct MeshRendererComponent
    {
        AssetHandle<MeshAsset> Mesh;
        glm::vec4 Color{ 1.0f };

        MeshRendererComponent() = default;
        MeshRendererComponent(AssetHandle<MeshAsset> mesh, const glm::vec4& color) : Mesh(mesh), Color(color)
        {

        }
    };

    struct MaterialComponent
    {
        std::shared_ptr<Material> Mat;

        MaterialComponent() = default;
        explicit MaterialComponent(std::shared_ptr<Material> material) : Mat(std::move(material))
        {

        }
    };

    enum class ProjectionType : uint8_t
    {
        Perspective = 0,
        Orthographic
    };

    struct CameraComponent
    {
        ProjectionType Projection = ProjectionType::Perspective;

        float FieldOfViewDegrees = 45.0f;
        float NearClip = 0.01f;
        float FarClip = 1000.0f;

        float OrthoSize = 10.0f;
        float OrthoNear = -1.0f;
        float OrthoFar = 1.0f;

        bool Primary = true;
        bool FixedAspectRatio = false;

        CameraComponent() = default;
    };

    enum class LightType : uint8_t
    {
        Directional = 0,
        Point,
        Spot
    };

    struct LightComponent
    {
        LightType Type = LightType::Directional;

        glm::vec3 Color{ 1.0f };
        float Intensity = 1.0f;

        float Range = 10.0f;

        float InnerConeAngleDegrees = 15.0f;
        float OuterConeAngleDegrees = 30.0f;

        bool CastShadows = false;

        LightComponent() = default;
        LightComponent(LightType type, const glm::vec3& color, float intensity) : Type(type), Color(color), Intensity(intensity)
        {

        }
    };

    enum class RigidBodyType : uint8_t
    {
        Static = 0,
        Dynamic,
        Kinematic
    };

    struct RigidBodyComponent
    {
        RigidBodyType Type = RigidBodyType::Static;

        float Mass = 1.0f;
        float LinearDrag = 0.0f;
        float AngularDrag = 0.05f;

        bool UseGravity = true;
        bool IsKinematic = false;

        glm::vec3 LinearVelocity{ 0.0f };
        glm::vec3 AngularVelocity{ 0.0f };

        void* RuntimeBody = nullptr;

        RigidBodyComponent() = default;
        explicit RigidBodyComponent(RigidBodyType type) : Type(type)
        {

        }
    };

    struct ScriptComponent
    {
        std::string ScriptPath;
        bool Active = true;

        void* RuntimeInstance = nullptr;

        ScriptComponent() = default;
        explicit ScriptComponent(const std::string& scriptPath) : ScriptPath(scriptPath)
        {

        }
    };
}