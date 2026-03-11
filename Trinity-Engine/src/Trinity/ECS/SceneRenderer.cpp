#include "Trinity/ECS/SceneRenderer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/ECS/Components.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Trinity
{
    SceneRenderer::RenderStats SceneRenderer::s_Stats{};
    float SceneRenderer::s_ViewportWidth = 1920.0f;
    float SceneRenderer::s_ViewportHeight = 1080.0f;

    void SceneRenderer::SetViewportSize(float width, float height)
    {
        s_ViewportWidth = width;
        s_ViewportHeight = height;
    }

    void SceneRenderer::Render(Scene& scene, const glm::mat4& view, const glm::mat4& projection)
    {
        ResetStats();
        SubmitMeshes(scene, view, projection);
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

                glm::mat4 l_Projection;
                const float l_Aspect = s_ViewportHeight > 0.0f ? s_ViewportWidth / s_ViewportHeight : 1.0f;

                if (camera.Projection == ProjectionType::Perspective)
                {
                    l_Projection = glm::perspective(glm::radians(camera.FieldOfViewDegrees), l_Aspect, camera.NearClip, camera.FarClip);
                }
                else
                {
                    const float l_HalfSize = camera.OrthoSize * 0.5f;
                    const float l_HalfWidth = l_HalfSize * l_Aspect;
                    l_Projection = glm::ortho(-l_HalfWidth, l_HalfWidth, -l_HalfSize, l_HalfSize, camera.OrthoNear, camera.OrthoFar);
                }

                SubmitMeshes(scene, l_View, l_Projection);
                l_Rendered = true;
            });
    }

    void SceneRenderer::SubmitMeshes(Scene& scene, const glm::mat4& view, const glm::mat4& projection)
    {
        auto& a_Registry = scene.GetRegistry();
        a_Registry.view<TransformComponent, MeshRendererComponent>().each([&](const TransformComponent& transform, const MeshRendererComponent& mesh)
            {
                RenderCommand::DrawMesh(mesh.Primitive, transform.GetTransform(), mesh.Color, view, projection);

                s_Stats.DrawCalls++;
                s_Stats.EntityCount++;
            });
    }

    const SceneRenderer::RenderStats& SceneRenderer::GetStats()
    {
        return s_Stats;
    }

    void SceneRenderer::ResetStats()
    {
        s_Stats.DrawCalls = 0;
        s_Stats.EntityCount = 0;
    }
}