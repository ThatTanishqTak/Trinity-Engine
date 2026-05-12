#pragma once

#include "Trinity/Renderer/Camera/Camera.h"
#include "Trinity/Renderer/Resources/Material.h"
#include "Trinity/Renderer/Resources/Texture.h"
#include "Trinity/Scene/Components/LightComponent.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    class Mesh;

    struct MeshDrawCommand
    {
        std::shared_ptr<Mesh> MeshRef;
        uint64_t MaterialHandle = 0;

        MaterialProperties Material;

        std::shared_ptr<Texture> AlbedoTexture;
        bool UseAlbedoTexture = false;

        std::shared_ptr<Texture> NormalTexture;
        bool UseNormalTexture = false;

        float Transform[16] = {};
    };

    struct SceneRenderData
    {
        DirectionalLight SunLight;
        glm::vec3 SunDirection = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.2f));
    };

    enum class AntiAliasingMode : uint8_t
    {
        None = 0,
        FXAA,
        TAA,
        FSR2
    };

    struct RenderPipelineSettings
    {
        bool LightingEnabled = true;
        bool SsaoEnabled = false;
        bool BloomEnabled = false;
        bool TaaEnabled = false;
        bool VolumetricsEnabled = false;
        bool SsrEnabled = false;
        bool RayTracedShadowsEnabled = false;

        AntiAliasingMode AntiAliasing = AntiAliasingMode::None;
        uint32_t ShadowCascadeCount = 1;
        uint32_t ShadowMapResolution = 2048;
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

    struct PushBlock
    {
        float Model[16];
        float ViewProjection[16];

        float BaseColor[4];

        float MaterialData[4];
        float EmissiveColorStrength[4];

        float TextureFlags[4];
    };

    class SceneRenderer
    {
    public:
        SceneRenderer();
        ~SceneRenderer();

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;

        void Initialize(uint32_t width, uint32_t height);
        void Shutdown();

        void SetSettings(const RenderPipelineSettings& settings);
        const RenderPipelineSettings& GetSettings() const { return m_Settings; }

        void BeginScene(const Camera& camera, const SceneRenderData& sceneData = {});
        void SubmitMesh(const MeshDrawCommand& command);
        void EndScene();

        void Render();
        void OnResize(uint32_t width, uint32_t height);

        std::shared_ptr<Texture> GetFinalOutput() const;

        const SceneRendererStats& GetStats() const;

        std::string DumpRenderGraphText() const;
        std::string DumpRenderGraphDot() const;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> m_Implementation;

        std::vector<MeshDrawCommand> m_DrawList;

        Camera m_Camera;
        SceneRenderData m_SceneData;
        RenderPipelineSettings m_Settings;

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };
}