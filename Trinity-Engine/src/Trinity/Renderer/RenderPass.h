#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace Trinity
{
    class CommandBuffer;
    class RenderGraph;
    struct RenderGraphContext;
    struct RenderPipelineSettings;

    struct FrameContext
    {
        uint32_t FrameIndex = 0;
        uint32_t SwapchainWidth = 0;
        uint32_t SwapchainHeight = 0;

        float DeltaTime = 0.0f;
        float TotalTime = 0.0f;

        glm::mat4 View = glm::mat4(1.0f);
        glm::mat4 Projection = glm::mat4(1.0f);
        glm::mat4 ViewProjection = glm::mat4(1.0f);
        glm::mat4 InverseView = glm::mat4(1.0f);
        glm::mat4 InverseProjection = glm::mat4(1.0f);
        glm::mat4 InverseViewProjection = glm::mat4(1.0f);

        glm::vec3 CameraPosition = glm::vec3(0.0f);
        glm::vec2 Jitter = glm::vec2(0.0f);
    };

    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual const char* GetName() const = 0;

        virtual void Setup(RenderGraph& graph, const FrameContext& frame) = 0;
        virtual void Execute(CommandBuffer& cmd, RenderGraphContext& ctx) = 0;

        virtual bool IsEnabled(const RenderPipelineSettings& settings) const
        {
            (void)settings;
            return true;
        }
    };
}