#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

#include <Trinity/Renderer/RHI/Handle.h>
#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Physics/DebugPhysicsDraw.h>

namespace Trinity
{
    class GraphicsDevice;
    class ShaderCompiler;
    class CommandList;

    // Immediate-mode line renderer recorded inside the scene pass: depth-tested against scene geometry but never written, so lines overlay the lit image before tonemapping
    class DebugLineStage
    {
    public:
        // Per-frame line cap; submissions beyond it are clamped with a warning
        static constexpr uint32_t MaxLines = 65536;

        DebugLineStage() = default;
        ~DebugLineStage() = default;

        DebugLineStage(const DebugLineStage&) = delete;
        DebugLineStage& operator=(const DebugLineStage&) = delete;

        bool Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory, Format colorFormat, Format depthFormat, uint32_t framesInFlight);
        void Shutdown();

        void Upload(uint32_t frameIndex, const std::vector<DebugLine>& lines);
        void Record(CommandList& commandList, uint32_t frameIndex, const glm::mat4& viewProjection);

    private:
        // Deliberately not MeshVertex: this stage only needs position + color, and sharing the mesh layout would couple it to every future MeshVertex change
        struct LineVertex
        {
            glm::vec3 Position;
            uint32_t Color;
        };

        GraphicsDevice* m_Device = nullptr;
        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;
        PipelineHandle m_Pipeline;

        std::vector<BufferHandle> m_VertexBuffers;
        std::vector<uint32_t> m_VertexCounts;
        std::vector<LineVertex> m_ScratchVertices;

        bool m_OverflowWarned = false;
    };
}