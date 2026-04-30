#pragma once

#include "Trinity/Renderer/Camera/Camera.h"
#include "Trinity/Scene/Components/LightComponent.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Trinity
{
    class Mesh;
    class RenderGraph;
    class Pipeline;
    class Texture;

    struct MeshDrawCommand
    {
        std::shared_ptr<Mesh> MeshRef;
        uint64_t MaterialHandle = 0;
        float Transform[16] = {};
    };

    struct SceneRenderData
    {
        DirectionalLight SunLight;
        glm::vec3 SunDirection = { 0.0f, -1.0f, 0.0f };
    };

    struct SceneRendererStats
    {
        uint32_t DrawCalls = 0;
        uint32_t VertexCount = 0;
        uint32_t IndexCount = 0;
        float GeometryPassMs = 0.0f;
        float ShadowPassMs = 0.0f;
        float TotalGPUMs = 0.0f;
        uint32_t GeometryBufferMemoryMB = 0;
    };

    class SceneRenderer
    {
    public:
        SceneRenderer();
        ~SceneRenderer();

        void Initialize(uint32_t width, uint32_t height);
        void Shutdown();

        void BeginScene(const Camera& camera, const SceneRenderData& sceneData);
        void SubmitMesh(const MeshDrawCommand& command);
        void EndScene();

        void Render();
        void OnResize(uint32_t width, uint32_t height);

        std::shared_ptr<Texture> GetFinalOutput() const;
        const SceneRendererStats& GetStats() const;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> m_Implementation;

        std::vector<MeshDrawCommand> m_DrawList;
        Camera m_Camera;
        SceneRenderData m_SceneData;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };
}