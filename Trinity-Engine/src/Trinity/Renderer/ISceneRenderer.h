#pragma once

#include <cstdint>
#include <memory>

namespace Trinity
{
    class Camera;
    class Texture;
    struct MeshDrawCommand;
    struct SceneRenderData;
    struct SceneRendererStats;

    class ISceneRenderer
    {
    public:
        virtual ~ISceneRenderer() = default;

        virtual void Initialize(uint32_t width, uint32_t height) = 0;
        virtual void Shutdown() = 0;

        virtual void BeginScene(const Camera& camera, const SceneRenderData& sceneData) = 0;
        virtual void SubmitMesh(const MeshDrawCommand& command) = 0;
        virtual void EndScene() = 0;

        virtual void Render() = 0;
        virtual void OnResize(uint32_t width, uint32_t height) = 0;

        virtual std::shared_ptr<Texture> GetFinalOutput() const = 0;
        virtual const SceneRendererStats& GetStats() const = 0;
    };

    std::unique_ptr<ISceneRenderer> CreateSceneRenderer();
    std::unique_ptr<ISceneRenderer> CreateVulkanSceneRenderer();
}