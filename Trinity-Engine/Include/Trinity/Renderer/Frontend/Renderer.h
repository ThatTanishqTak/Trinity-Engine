#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <filesystem>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/RHI/Swapchain.h>
#include <Trinity/Renderer/RHI/CommandList.h>
#include <Trinity/Core/Timer.h>
#include <Trinity/Renderer/Frontend/Camera.h>
#include <Trinity/Renderer/Shaders/ShaderCompiler.h>
#include <Trinity/Renderer/Textures/TextureManager.h>
#include <Trinity/Renderer/Meshes/MeshLibrary.h>

namespace Trinity
{
    class FileSystem;
    class Scene;

    class Renderer
    {
    public:
        Renderer(GraphicsDevice& device, Swapchain& swapchain, FileSystem& fileSystem);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool Initialize();
        void Shutdown();

        void RenderFrame(Scene& scene, const Camera& camera);
        void Resize(uint32_t width, uint32_t height);

        MeshLibrary& GetMeshLibrary() { return m_MeshLibrary; }

    private:
        bool CreatePipeline();
        bool BuildPipeline(ShaderHandle& vertexShader, ShaderHandle& fragmentShader, PipelineHandle& pipeline);
        void ReloadShaders();
        void CheckHotReload();
        bool CreateTextureResources();
        bool CreateDepthTexture(uint32_t width, uint32_t height);
        void DestroyDepthTexture();

    private:
        GraphicsDevice& m_Device;
        Swapchain& m_Swapchain;
        FileSystem& m_FileSystem;
        ShaderCompiler m_ShaderCompiler;
        TextureManager m_TextureManager;

        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;
        PipelineHandle m_Pipeline;
        MeshLibrary m_MeshLibrary;

        TextureHandle m_DepthTexture;
        TextureHandle m_Texture;
        SamplerHandle m_Sampler;

        std::vector<std::unique_ptr<CommandList>> m_CommandLists;
        uint32_t m_FrameIndex = 0;

        Timer m_Timer;

        std::filesystem::path m_ShaderSourcePath;
        std::filesystem::file_time_type m_ShaderWriteTime;
        float m_LastReloadCheck = 0.0f;
        bool m_Minimized = false;
    };
}