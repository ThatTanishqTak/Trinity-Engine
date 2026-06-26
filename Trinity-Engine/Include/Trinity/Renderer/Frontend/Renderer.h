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
#include <Trinity/Renderer/PostProcess/PostProcessStage.h>
#include <Trinity/Renderer/Graph/RenderGraph.h>

namespace Trinity
{
    class FileSystem;
    class Scene;
    class ImGuiLayer;
    class AssetDatabase;

    class Renderer
    {
    public:
        Renderer(GraphicsDevice& device, Swapchain& swapchain, FileSystem& fileSystem);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool Initialize();
        void Shutdown();

        void RenderFrame(Scene& scene, AssetDatabase& assetDatabase, const Camera& camera, ImGuiLayer* imgui = nullptr);
        void Resize(uint32_t width, uint32_t height);

        MeshLibrary& GetMeshLibrary() { return m_MeshLibrary; }
        TextureManager& GetTextureManager() { return m_TextureManager; }

        void SetViewportSize(uint32_t width, uint32_t height);
        uint64_t GetViewportTextureID() const { return m_ViewportTextureID; }
        const RenderGraph& GetRenderGraph() const { return m_RenderGraph; }
        void ApplyViewportResize();

    private:
        bool CreatePipeline();
        bool BuildPipeline(ShaderHandle& vertexShader, ShaderHandle& fragmentShader, PipelineHandle& pipeline);
        void ReloadShaders();
        void CheckHotReload();
        bool CreateTextureResources();
        bool CreateSceneTargets(uint32_t width, uint32_t height);
        void DestroySceneTargets();
        bool CreateViewportOutput(uint32_t width, uint32_t height);
        void DestroyViewportOutput();
        void DrawScene(CommandList& commandList, Scene& scene, AssetDatabase& assetDatabase, const Camera& camera);

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

        PostProcessStage m_PostProcess;
        RenderGraph m_RenderGraph;

        std::vector<BufferHandle> m_FrameUniforms;

        TextureHandle m_SceneColor;
        TextureHandle m_SceneDepth;

        uint32_t m_RenderWidth = 0;
        uint32_t m_RenderHeight = 0;

        TextureHandle m_ViewportColor;
        bool m_ViewportActive = false;
        uint32_t m_PendingWidth = 0;
        uint32_t m_PendingHeight = 0;
        bool m_ViewportDirty = false;
        uint64_t m_ViewportTextureID = 0;

        float m_Exposure = 1.0f;

        std::vector<std::unique_ptr<CommandList>> m_CommandLists;
        uint32_t m_FrameIndex = 0;

        Timer m_Timer;

        std::filesystem::path m_ShaderSourcePath;
        std::filesystem::file_time_type m_ShaderWriteTime;
        float m_LastReloadCheck = 0.0f;
        bool m_Minimized = false;
    };
}