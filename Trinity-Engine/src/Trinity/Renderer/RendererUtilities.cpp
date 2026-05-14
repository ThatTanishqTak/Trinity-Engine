#include "Trinity/Renderer/RendererUtilities.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Scene/Components/LightComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Utilities/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace Trinity
{
    namespace RendererUtilities
    {
        glm::mat4 Perspective(float fovDegrees, float aspectRatio, float nearClip, float farClip)
        {
            glm::mat4 l_Projection = glm::perspective(glm::radians(fovDegrees), aspectRatio, nearClip, farClip);
            l_Projection[1][1] *= -1.0f;

            return l_Projection;
        }

        glm::mat4 Orthographic(float left, float right, float bottom, float top, float nearClip, float farClip)
        {
            glm::mat4 l_Projection = glm::ortho(left, right, bottom, top, nearClip, farClip);
            l_Projection[1][1] *= -1.0f;

            return l_Projection;
        }

        void ComputeFrustumCornersWorld(const Camera& camera, glm::vec3 outCorners[8])
        {
            const glm::mat4 l_InverseViewProjection = glm::inverse(camera.GetViewProjectionMatrix());
            ComputeFrustumCornersWorld(l_InverseViewProjection, outCorners);
        }

        void ComputeFrustumCornersWorld(const glm::mat4& inverseViewProjection, glm::vec3 outCorners[8])
        {
            const glm::vec4 l_NdcCorners[8] =
            {
                { -1.0f, -1.0f, 0.0f, 1.0f },
                {  1.0f, -1.0f, 0.0f, 1.0f },
                { -1.0f,  1.0f, 0.0f, 1.0f },
                {  1.0f,  1.0f, 0.0f, 1.0f },
                { -1.0f, -1.0f, 1.0f, 1.0f },
                {  1.0f, -1.0f, 1.0f, 1.0f },
                { -1.0f,  1.0f, 1.0f, 1.0f },
                {  1.0f,  1.0f, 1.0f, 1.0f },
            };

            for (int it_Corner = 0; it_Corner < 8; ++it_Corner)
            {
                const glm::vec4 l_World = inverseViewProjection * l_NdcCorners[it_Corner];
                outCorners[it_Corner] = glm::vec3(l_World) / l_World.w;
            }
        }

        glm::mat4 ComputeDirectionalLightViewProjection(const Camera& camera, const glm::vec3& sunDirection, uint32_t shadowMapResolution, float shadowRadius)
        {
            if (shadowMapResolution == 0 || shadowRadius <= 0.0f)
            {
                TR_CORE_WARN("ComputeDirectionalLightViewProjection called with invalid parameters (resolution={}, radius={})", shadowMapResolution, shadowRadius);

                return glm::mat4(1.0f);
            }

            const glm::vec3 l_LightDirection = glm::normalize(sunDirection);
            const glm::vec3 l_WorldUp = std::abs(l_LightDirection.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

            const glm::vec3 l_Right = glm::normalize(glm::cross(l_WorldUp, l_LightDirection));
            const glm::vec3 l_TrueUp = glm::normalize(glm::cross(l_LightDirection, l_Right));

            const glm::vec3 l_SphereCentreWorld = camera.GetPosition();

            const float l_TexelSize = (2.0f * shadowRadius) / static_cast<float>(shadowMapResolution);

            const float l_CentreRight = glm::dot(l_SphereCentreWorld, l_Right);
            const float l_CentreUp = glm::dot(l_SphereCentreWorld, l_TrueUp);
            const float l_CentreForward = glm::dot(l_SphereCentreWorld, l_LightDirection);

            const float l_SnappedRight = std::floor(l_CentreRight / l_TexelSize) * l_TexelSize;
            const float l_SnappedUp = std::floor(l_CentreUp / l_TexelSize) * l_TexelSize;

            const glm::vec3 l_SnappedCentre = l_Right * l_SnappedRight + l_TrueUp * l_SnappedUp + l_LightDirection * l_CentreForward;

            const glm::vec3 l_LightEye = l_SnappedCentre - l_LightDirection * shadowRadius;

            const glm::mat4 l_LightView = glm::lookAt(l_LightEye, l_SnappedCentre, l_TrueUp);
            const glm::mat4 l_LightProjection = Orthographic(-shadowRadius, shadowRadius, -shadowRadius, shadowRadius, 0.0f, 2.0f * shadowRadius);

            TR_CORE_TRACE("Shadow LVP centre=({:.2f},{:.2f},{:.2f}) snapped=({:.2f},{:.2f}) radius={:.2f} texel={:.4f}", l_SphereCentreWorld.x, l_SphereCentreWorld.y, l_SphereCentreWorld.z, l_SnappedRight, l_SnappedUp, shadowRadius, l_TexelSize);

            return l_LightProjection * l_LightView;
        }

        glm::vec3 RotateDirection(const glm::vec3& rotationRadians, const glm::vec3& baseDirection)
        {
            glm::mat4 l_Rotation = glm::mat4(1.0f);
            l_Rotation = glm::rotate(l_Rotation, rotationRadians.x, glm::vec3(1.0f, 0.0f, 0.0f));
            l_Rotation = glm::rotate(l_Rotation, rotationRadians.y, glm::vec3(0.0f, 1.0f, 0.0f));
            l_Rotation = glm::rotate(l_Rotation, rotationRadians.z, glm::vec3(0.0f, 0.0f, 1.0f));

            return glm::normalize(glm::vec3(l_Rotation * glm::vec4(baseDirection, 0.0f)));
        }

        namespace
        {
            std::shared_ptr<Texture> CreateSolidColorTexture(uint32_t rgbaPixel, const char* debugName)
            {
                TextureSpecification l_TextureSpecification{};
                l_TextureSpecification.Width = 1;
                l_TextureSpecification.Height = 1;
                l_TextureSpecification.Format = TextureFormat::RGBA8;
                l_TextureSpecification.Usage = TextureUsage::Sampled | TextureUsage::TransferDestination;
                l_TextureSpecification.DebugName = debugName;

                auto l_Texture = Renderer::GetAPI().CreateTexture(l_TextureSpecification);

                if (l_Texture)
                {
                    l_Texture->Upload(&rgbaPixel, sizeof(rgbaPixel));

                    TR_CORE_INFO("Created default texture '{}' (1x1, 0x{:08X})", debugName, rgbaPixel);
                }
                else
                {
                    TR_CORE_ERROR("Failed to create default texture '{}'", debugName);
                }

                return l_Texture;
            }
        }

        std::shared_ptr<Texture> CreateWhiteTexture()
        {
            return CreateSolidColorTexture(0xFFFFFFFFu, "DefaultWhiteTexture");
        }

        std::shared_ptr<Texture> CreateBlackTexture()
        {
            return CreateSolidColorTexture(0xFF000000u, "DefaultBlackTexture");
        }

        std::shared_ptr<Texture> CreateDefaultNormalTexture()
        {
            return CreateSolidColorTexture(0xFFFF8080u, "DefaultNormalTexture");
        }

        std::shared_ptr<Texture> CreateCheckerboardTexture(uint32_t width, uint32_t height)
        {
            if (width == 0 || height == 0)
            {
                TR_CORE_WARN("CreateCheckerboardTexture called with zero dimensions (width={}, height={})", width, height);

                return nullptr;
            }

            TextureSpecification l_TextureSpecification{};
            l_TextureSpecification.Width = width;
            l_TextureSpecification.Height = height;
            l_TextureSpecification.Format = TextureFormat::RGBA8;
            l_TextureSpecification.Usage = TextureUsage::Sampled | TextureUsage::TransferDestination;
            l_TextureSpecification.DebugName = "DefaultCheckerboardTexture";

            auto l_Texture = Renderer::GetAPI().CreateTexture(l_TextureSpecification);

            if (!l_Texture)
            {
                TR_CORE_ERROR("Failed to create checkerboard texture");

                return nullptr;
            }

            std::vector<uint32_t> l_Pixels(static_cast<size_t>(width) * static_cast<size_t>(height));

            for (uint32_t it_Y = 0; it_Y < height; ++it_Y)
            {
                for (uint32_t it_X = 0; it_X < width; ++it_X)
                {
                    const bool l_IsLight = ((it_X / 8u) + (it_Y / 8u)) % 2u == 0u;
                    l_Pixels[static_cast<size_t>(it_Y) * width + it_X] = l_IsLight ? 0xFFCCCCCCu : 0xFF333333u;
                }
            }

            l_Texture->Upload(l_Pixels.data(), l_Pixels.size() * sizeof(uint32_t));

            TR_CORE_INFO("Created checkerboard texture ({}x{})", width, height);

            return l_Texture;
        }

        DirectionalSunData CollectDirectionalSun(const entt::registry& registry)
        {
            DirectionalSunData l_Result{};

            const auto l_View = registry.view<TransformComponent, LightComponent>();

            for (const auto it_Entity : l_View)
            {
                const auto& a_Light = l_View.get<LightComponent>(it_Entity);

                if (!std::holds_alternative<DirectionalLight>(a_Light.Data))
                {
                    continue;
                }

                const auto& a_Transform = l_View.get<TransformComponent>(it_Entity);
                const auto& a_Directional = std::get<DirectionalLight>(a_Light.Data);

                l_Result.Direction = RotateDirection(a_Transform.Rotation, glm::vec3(0.0f, -1.0f, 0.0f));
                l_Result.Color = a_Directional.Color;
                l_Result.Intensity = a_Directional.Intensity;
                l_Result.Valid = true;

                TR_CORE_TRACE("Collected directional sun: direction=({:.2f},{:.2f},{:.2f}) intensity={:.2f}", l_Result.Direction.x, l_Result.Direction.y, l_Result.Direction.z, l_Result.Intensity);

                return l_Result;
            }

            TR_CORE_TRACE("No directional sun present in registry; using fallback direction");

            return l_Result;
        }
    }
}