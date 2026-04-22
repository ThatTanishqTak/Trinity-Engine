#pragma once

#include "Trinity/Renderer/ISceneRenderer.h"
#include "Trinity/Renderer/SceneRenderer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace Trinity
{
    class VulkanSampler;
    class VulkanTexture;

    class VulkanSceneRenderer final : public ISceneRenderer
    {
    public:
        VulkanSceneRenderer();
        ~VulkanSceneRenderer() override;

        void Initialize(uint32_t width, uint32_t height) override;
        void Shutdown() override;
        void BeginScene(const Camera& camera, const SceneRenderData& sceneData) override;
        void SubmitMesh(const MeshDrawCommand& command) override;
        void EndScene() override;
        void Render() override;
        void OnResize(uint32_t width, uint32_t height) override;

        std::shared_ptr<Texture> GetFinalOutput() const override;
        const SceneRendererStats& GetStats() const override;

    private:
        bool BindMeshBuffers(VkCommandBuffer commandBuffer, const MeshDrawCommand& command) const;
        void DrawMeshCommand(VkCommandBuffer commandBuffer, const MeshDrawCommand& command) const;

        VkDescriptorSet GetOrCreateGeometryMaterialSet(Texture* texture);

        void BindGeometryMaterialSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Texture* texture);
        void UpdateLightingDescriptorSet(uint32_t frameIndex, VulkanSampler* sampler, VulkanTexture* albedo, VulkanTexture* normal, VulkanTexture* mra, VulkanTexture* depth, VulkanTexture* shadow);
        void SetupFallbackTextureAndSampler();

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