#include "Trinity/ECS/SceneRenderer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/ECS/Components.h"

namespace Trinity
{
    void SceneRenderer::Render(Scene& scene, const glm::mat4& view, const glm::mat4& projection)
    {
        auto& registry = scene.GetRegistry();

        registry.view<TransformComponent, MeshRendererComponent>().each([&](entt::entity entity, TransformComponent& transform, MeshRendererComponent& mesh)
            {
                RenderCommand::DrawMesh(mesh.Primitive, transform.Translation, mesh.Color, view, projection);
            }
        );
    }
}