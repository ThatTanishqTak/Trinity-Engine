#include <Trinity/Renderer/Debug/DebugLineStage.h>

#include <cstddef>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/RHI/CommandList.h>
#include <Trinity/Renderer/RHI/Pipeline.h>
#include <Trinity/Renderer/RHI/Shader.h>
#include <Trinity/Renderer/RHI/Buffer.h>
#include <Trinity/Renderer/Shaders/ShaderCompiler.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    struct DebugLinePush
    {
        glm::mat4 ViewProjection;
    };

    bool DebugLineStage::Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory, Format colorFormat, Format depthFormat, uint32_t framesInFlight)
    {
        m_Device = &device;

        ShaderCompileResult l_VertexResult = compiler.Compile(shaderDirectory, "DebugLine", "vertexMain");
        ShaderCompileResult l_FragmentResult = compiler.Compile(shaderDirectory, "DebugLine", "fragmentMain");
        if (!l_VertexResult.Success || !l_FragmentResult.Success)
        {

            return false;
        }

        ShaderDescription l_VertexDescription;
        l_VertexDescription.Stage = ShaderStage::Vertex;
        l_VertexDescription.Bytecode = l_VertexResult.SPIRV;
        l_VertexDescription.EntryPoint = "vertexMain";
        l_VertexDescription.DebugName = "DebugLine.vertexMain";
        m_VertexShader = device.CreateShader(l_VertexDescription);

        ShaderDescription l_FragmentDescription;
        l_FragmentDescription.Stage = ShaderStage::Fragment;
        l_FragmentDescription.Bytecode = l_FragmentResult.SPIRV;
        l_FragmentDescription.EntryPoint = "fragmentMain";
        l_FragmentDescription.DebugName = "DebugLine.fragmentMain";
        m_FragmentShader = device.CreateShader(l_FragmentDescription);

        if (!m_VertexShader.IsValid() || !m_FragmentShader.IsValid())
        {
            Shutdown();

            return false;
        }

        VertexLayout l_Layout;
        l_Layout.Stride = sizeof(LineVertex);
        l_Layout.Attributes = {
            { 0, static_cast<uint32_t>(offsetof(LineVertex, Position)), Format::RGB32_SFLOAT },
            { 1, static_cast<uint32_t>(offsetof(LineVertex, Color)), Format::RGBA8_UNORM }
        };

        PipelineDescription l_PipelineDescription;
        l_PipelineDescription.VertexShader = m_VertexShader;
        l_PipelineDescription.FragmentShader = m_FragmentShader;
        l_PipelineDescription.Vertex = l_Layout;
        l_PipelineDescription.Topology = PrimitiveTopology::LineList;
        l_PipelineDescription.Rasterizer.Cull = CullMode::None;
        l_PipelineDescription.DepthStencil.DepthTest = true;
        l_PipelineDescription.DepthStencil.DepthWrite = false;
        l_PipelineDescription.DepthFormat = depthFormat;
        l_PipelineDescription.PushConstantSize = sizeof(DebugLinePush);
        l_PipelineDescription.ColorFormats = { colorFormat };
        l_PipelineDescription.DebugName = "DebugLine";

        m_Pipeline = device.CreatePipeline(l_PipelineDescription);
        if (!m_Pipeline.IsValid())
        {
            Shutdown();

            return false;
        }

        m_VertexBuffers.reserve(framesInFlight);
        m_VertexCounts.assign(framesInFlight, 0);
        for (uint32_t l_Index = 0; l_Index < framesInFlight; ++l_Index)
        {
            BufferDescription l_BufferDescription;
            l_BufferDescription.Size = static_cast<uint64_t>(MaxLines) * 2u * sizeof(LineVertex);
            l_BufferDescription.Usage = BufferUsage::Vertex;
            l_BufferDescription.Memory = MemoryUsage::CpuToGpu;
            l_BufferDescription.DebugName = "DebugLineVertices";

            BufferHandle l_Buffer = device.CreateBuffer(l_BufferDescription);
            if (!l_Buffer.IsValid())
            {
                Shutdown();

                return false;
            }

            m_VertexBuffers.push_back(l_Buffer);
        }

        return true;
    }

    void DebugLineStage::Shutdown()
    {
        if (m_Device == nullptr)
        {
            return;
        }

        for (BufferHandle& it_Buffer : m_VertexBuffers)
        {
            if (it_Buffer.IsValid())
            {
                m_Device->DestroyBuffer(it_Buffer);
            }
        }
        m_VertexBuffers.clear();
        m_VertexCounts.clear();

        if (m_Pipeline.IsValid())
        {
            m_Device->DestroyPipeline(m_Pipeline);
            m_Pipeline = PipelineHandle{};
        }

        if (m_FragmentShader.IsValid())
        {
            m_Device->DestroyShader(m_FragmentShader);
            m_FragmentShader = ShaderHandle{};
        }

        if (m_VertexShader.IsValid())
        {
            m_Device->DestroyShader(m_VertexShader);
            m_VertexShader = ShaderHandle{};
        }
    }

    void DebugLineStage::Upload(uint32_t frameIndex, const std::vector<DebugLine>& lines)
    {
        if (m_Device == nullptr || frameIndex >= m_VertexBuffers.size())
        {
            return;
        }

        uint32_t l_LineCount = static_cast<uint32_t>(lines.size());
        if (l_LineCount > MaxLines)
        {
            if (!m_OverflowWarned)
            {
                TR_CORE_WARN("Debug line submission ({} lines) exceeds the per-frame cap ({}); excess lines are dropped", l_LineCount, MaxLines);
                m_OverflowWarned = true;
            }

            l_LineCount = MaxLines;
        }
        else
        {
            m_OverflowWarned = false;
        }

        m_VertexCounts[frameIndex] = l_LineCount * 2;
        if (l_LineCount == 0)
        {
            return;
        }

        m_ScratchVertices.clear();
        m_ScratchVertices.reserve(static_cast<size_t>(l_LineCount) * 2);
        for (uint32_t l_Index = 0; l_Index < l_LineCount; ++l_Index)
        {
            const DebugLine& l_Line = lines[l_Index];
            m_ScratchVertices.push_back(LineVertex{ l_Line.Start, l_Line.Color });
            m_ScratchVertices.push_back(LineVertex{ l_Line.End, l_Line.Color });
        }

        m_Device->UpdateBuffer(m_VertexBuffers[frameIndex], m_ScratchVertices.data(), m_ScratchVertices.size() * sizeof(LineVertex), 0);
    }

    void DebugLineStage::Record(CommandList& commandList, uint32_t frameIndex, const glm::mat4& viewProjection)
    {
        if (!m_Pipeline.IsValid() || frameIndex >= m_VertexBuffers.size() || m_VertexCounts[frameIndex] == 0)
        {
            return;
        }

        DebugLinePush l_Push;
        l_Push.ViewProjection = viewProjection;

        commandList.BindPipeline(m_Pipeline);
        commandList.BindVertexBuffer(m_VertexBuffers[frameIndex]);
        commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(DebugLinePush)), &l_Push);
        commandList.Draw(m_VertexCounts[frameIndex], 1, 0, 0);
    }
}