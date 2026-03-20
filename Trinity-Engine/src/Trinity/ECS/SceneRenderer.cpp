#include "Trinity/ECS/SceneRenderer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderGraph.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanShadowPass.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanGeometryPass.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanLightingPass.h"
#include "Trinity/ECS/Components.h"
#include "Trinity/Assets/AssetManager.h"
#include "Trinity/Assets/MeshAsset.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

namespace Trinity
{
    SceneRenderer::RenderStats SceneRenderer::s_Stats{};
    float SceneRenderer::s_ViewportWidth = 1920.0f;
    float SceneRenderer::s_ViewportHeight = 1080.0f;

    std::vector<LightData> SceneRenderer::s_LightBuffer;
    glm::mat4 SceneRenderer::s_DirectionalLightSpaceMatrix = glm::mat4(1.0f);
    bool SceneRenderer::s_HasShadowCaster = false;

    void SceneRenderer::SetViewportSize(float width, float height)
    {
        s_ViewportWidth = width;
        s_ViewportHeight = height;
    }

    void SceneRenderer::Render(Scene& scene, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar)
    {
        ResetStats();

        VulkanRenderGraph& l_Graph = RenderCommand::GetRenderGraph();

        // Reset per-frame submission state on each pass
        if (auto* l_Shadow = l_Graph.GetPass<VulkanShadowPass>())
        {
            l_Shadow->Reset();
        }

        if (auto* l_Geom = l_Graph.GetPass<VulkanGeometryPass>())
        {
            l_Geom->Reset();
        }

        CollectLights(scene);
        SubmitShadowDraws(scene);
        SubmitGeometryDraws(scene, view, projection);
        SubmitLighting(view, projection, cameraPosition, cameraNear, cameraFar);
    }

    void SceneRenderer::Render(Scene& scene)
    {
        ResetStats();

        auto& a_Registry = scene.GetRegistry();
        bool l_Rendered = false;

        a_Registry.view<TransformComponent, CameraComponent>().each([&](const TransformComponent& transform, const CameraComponent& camera)
        {
            if (l_Rendered || !camera.Primary)
            {
                return;
            }

            const glm::mat4 l_View = glm::inverse(transform.GetTransform());
            const float l_Aspect = s_ViewportHeight > 0.0f ? s_ViewportWidth / s_ViewportHeight : 1.0f;

            glm::mat4 l_Projection;
            if (camera.Projection == ProjectionType::Perspective)
            {
                l_Projection = glm::perspective(glm::radians(camera.FieldOfViewDegrees), l_Aspect, camera.NearClip, camera.FarClip);
            }
            else
            {
                const float l_Half = camera.OrthoSize * 0.5f;
                l_Projection = glm::ortho(-l_Half * l_Aspect, l_Half * l_Aspect, -l_Half, l_Half, camera.OrthoNear, camera.OrthoFar);
            }

            Render(scene, l_View, l_Projection, transform.Translation, camera.NearClip, camera.FarClip);
            l_Rendered = true;
        });
    }

    void SceneRenderer::CollectLights(Scene& scene)
    {
        s_LightBuffer.clear();
        s_HasShadowCaster = false;

        auto& a_Registry = scene.GetRegistry();

        a_Registry.view<TransformComponent, LightComponent>().each([&](const TransformComponent& transform, const LightComponent& light)
        {
            if (s_LightBuffer.size() >= MaxLights)
            {
                return;
            }

            LightData l_Data{};

            if (light.Type == LightType::Directional)
            {
                const glm::quat l_Rot = glm::quat(transform.Rotation);
                const glm::vec3 l_Dir = glm::normalize(l_Rot * glm::vec3(0.0f, -1.0f, 0.0f));

                l_Data.PositionAndType = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                l_Data.DirectionAndIntensity = glm::vec4(l_Dir, light.Intensity);
                l_Data.ColorAndRange = glm::vec4(light.Color, 0.0f);
                l_Data.SpotAngles = glm::vec4(0.0f);

                if (light.CastShadows && !s_HasShadowCaster)
                {
                    constexpr float l_OrthoSize = 20.0f;
                    constexpr float l_Near = 0.1f;
                    constexpr float l_Far = 100.0f;

                    const glm::mat4 l_LightProj = glm::ortho(-l_OrthoSize, l_OrthoSize, -l_OrthoSize, l_OrthoSize, l_Near, l_Far);
                    const glm::mat4 l_LightView = glm::lookAt(-l_Dir * (l_Far * 0.3f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    s_DirectionalLightSpaceMatrix = l_LightProj * l_LightView;
                    s_HasShadowCaster = true;

                    if (auto* l_Shadow = RenderCommand::GetRenderGraph().GetPass<VulkanShadowPass>())
                    {
                        l_Shadow->SetLightSpaceMatrix(s_DirectionalLightSpaceMatrix);
                    }
                }
            }
            else if (light.Type == LightType::Point)
            {
                l_Data.PositionAndType = glm::vec4(transform.Translation, 1.0f);
                l_Data.DirectionAndIntensity = glm::vec4(0.0f, 0.0f, 0.0f, light.Intensity);
                l_Data.ColorAndRange = glm::vec4(light.Color, light.Range);
                l_Data.SpotAngles = glm::vec4(0.0f);
            }
            else // Spot
            {
                const glm::quat l_Rot = glm::quat(transform.Rotation);
                const glm::vec3 l_Dir = glm::normalize(l_Rot * glm::vec3(0.0f, -1.0f, 0.0f));

                l_Data.PositionAndType = glm::vec4(transform.Translation, 2.0f);
                l_Data.DirectionAndIntensity = glm::vec4(l_Dir, light.Intensity);
                l_Data.ColorAndRange = glm::vec4(light.Color, light.Range);
                l_Data.SpotAngles = glm::vec4(std::cos(glm::radians(light.InnerConeAngleDegrees)), std::cos(glm::radians(light.OuterConeAngleDegrees)), 0.0f, 0.0f);
            }

            s_LightBuffer.push_back(l_Data);
        });

        s_Stats.LightCount = static_cast<uint32_t>(s_LightBuffer.size());
    }

    void SceneRenderer::SubmitShadowDraws(Scene& scene)
    {
        if (!s_HasShadowCaster)
        {
            return;
        }

        auto* l_ShadowPass = RenderCommand::GetRenderGraph().GetPass<VulkanShadowPass>();
        if (!l_ShadowPass)
        {
            return;
        }

        AssetManager& l_AssetManager = AssetManager::Get();
        auto& a_Registry = scene.GetRegistry();

        a_Registry.view<TransformComponent, MeshRendererComponent>().each([&](const TransformComponent& transform, const MeshRendererComponent& meshRenderer)
        {
            if (!meshRenderer.Mesh)
            {
                return;
            }

            std::shared_ptr<MeshAsset> l_Mesh = l_AssetManager.Resolve(meshRenderer.Mesh);
            if (!l_Mesh || !l_Mesh->IsValid())
            {
                return;
            }

            const glm::mat4 l_LightSpaceMVP = s_DirectionalLightSpaceMatrix * transform.GetTransform();
            l_ShadowPass->Submit(l_Mesh->GetVertexBuffer(), l_Mesh->GetIndexBuffer(), l_Mesh->GetIndexCount(), l_LightSpaceMVP);
        });
    }

    void SceneRenderer::SubmitGeometryDraws(Scene& scene, const glm::mat4& view, const glm::mat4& projection)
    {
        auto* l_GeomPass = RenderCommand::GetRenderGraph().GetPass<VulkanGeometryPass>();
        if (!l_GeomPass)
        {
            return;
        }

        AssetManager& l_AssetManager = AssetManager::Get();
        auto& a_Registry = scene.GetRegistry();
        const glm::mat4 l_VP = projection * view;

        a_Registry.view<TransformComponent, MeshRendererComponent>().each([&](entt::entity entity, const TransformComponent& transform, const MeshRendererComponent& meshRenderer)
        {
            if (!meshRenderer.Mesh)
            {
                return;
            }

            std::shared_ptr<MeshAsset> l_Mesh = l_AssetManager.Resolve(meshRenderer.Mesh);
            if (!l_Mesh || !l_Mesh->IsValid())
            {
                return;
            }

            GeometryDrawCommand l_Command{};
            l_Command.VB = &l_Mesh->GetVertexBuffer();
            l_Command.IB = &l_Mesh->GetIndexBuffer();
            l_Command.IndexCount = l_Mesh->GetIndexCount();
            l_Command.Model = transform.GetTransform();
            l_Command.ViewProjection = l_VP;
            l_Command.Color = meshRenderer.Color;
            l_Command.AlbedoTexture = nullptr;

            l_GeomPass->Submit(l_Command);

            s_Stats.DrawCalls++;
            s_Stats.EntityCount++;
        });
    }

    void SceneRenderer::SubmitLighting(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar)
    {
        auto* l_LightPass = RenderCommand::GetRenderGraph().GetPass<VulkanLightingPass>();
        if (!l_LightPass)
        {
            return;
        }

        LightingSubmitData l_Data{};
        l_Data.UniformData.LightCount = static_cast<uint32_t>(s_LightBuffer.size());
        l_Data.UniformData.ShadowsEnabled = s_HasShadowCaster ? 1u : 0u;
        l_Data.UniformData.DirectionalLightSpaceMatrix = s_DirectionalLightSpaceMatrix;

        for (uint32_t i = 0; i < l_Data.UniformData.LightCount; ++i)
        {
            l_Data.UniformData.Lights[i] = s_LightBuffer[i];
        }

        l_Data.InvViewProjection = glm::inverse(projection * view);
        l_Data.CameraPosition = cameraPosition;
        l_Data.CameraNear = cameraNear;
        l_Data.CameraFar = cameraFar;

        l_LightPass->Submit(l_Data);
    }

    const SceneRenderer::RenderStats& SceneRenderer::GetStats()
    {
        return s_Stats;
    }

    void SceneRenderer::ResetStats()
    {
        s_Stats.DrawCalls = 0;
        s_Stats.EntityCount = 0;
        s_Stats.LightCount = 0;
    }
}