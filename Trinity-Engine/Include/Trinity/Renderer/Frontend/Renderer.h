#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/RHI/Swapchain.h>
#include <Trinity/Renderer/RHI/CommandList.h>
#include <Trinity/Core/Timer.h>
#include <Trinity/Renderer/Frontend/Camera.h>

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
        bool CreateDepthTexture(uint32_t width, uint32_t height);
        void DestroyDepthTexture();

    private:
        GraphicsDevice& m_Device;
        Swapchain& m_Swapchain;
        FileSystem& m_FileSystem;

        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;
        PipelineHandle m_Pipeline;
        BufferHandle m_VertexBuffer;
        BufferHandle m_IndexBuffer;
        uint32_t m_IndexCount = 0;

        TextureHandle m_DepthTexture;

        std::vector<std::unique_ptr<CommandList>> m_CommandLists;
        uint32_t m_FrameIndex = 0;

        Camera m_Camera;
        Timer m_Timer;
    };
}