#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/RHI/Swapchain.h>
#include <Trinity/Renderer/RHI/CommandList.h>

namespace Trinity
{
    class FileSystem;

    class Renderer
    {
    public:
        Renderer(GraphicsDevice& device, Swapchain& swapchain, FileSystem& fileSystem);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool Initialize();
        void Shutdown();

        void RenderFrame();
        void Resize(uint32_t width, uint32_t height);

    private:
        bool CreatePipeline();
        bool CreateGeometry();

    private:
        GraphicsDevice& m_Device;
        Swapchain& m_Swapchain;
        FileSystem& m_FileSystem;

        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;
        PipelineHandle m_Pipeline;
        BufferHandle m_VertexBuffer;
        uint32_t m_VertexCount = 0;

        std::vector<std::unique_ptr<CommandList>> m_CommandLists;
        uint32_t m_FrameIndex = 0;
    };
}