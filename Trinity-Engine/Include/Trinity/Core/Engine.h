#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <Trinity/Core/Timestep.h>
#include <Trinity/ImGui/ImGuiLayer.h>

namespace Trinity
{
    class IPlatform;
    class GraphicsDevice;
    class Swapchain;
    class Renderer;
    class MeshLibrary;
    class AssetDatabase;
    class AudioEngine;
    class Scene;
    class EditorCamera;
    class Camera;

    struct NativeWindowHandle;

    class Engine
    {
    public:
        Engine();
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        bool Initialize(const std::string& applicationName);
        bool InitializeRenderer(const NativeWindowHandle& window, const std::string& applicationName);
        void Update(Timestep timestep);
        void Shutdown();

        void RenderFrame();
        void Resize(uint32_t width, uint32_t height);

        void SetViewportSize(uint32_t width, uint32_t height);
        uint64_t GetViewportTextureID() const;
        void SetViewportInteractive(bool interactive);

        void InitializeImGui();
        void BeginImGuiFrame();

        IPlatform& GetPlatform() { return *m_Platform; }
        bool HasPlatform() const { return m_Platform != nullptr; }

        GraphicsDevice& GetDevice() { return *m_Device; }
        bool HasDevice() const { return m_Device != nullptr; }

        Swapchain& GetSwapchain() { return *m_Swapchain; }
        bool HasSwapchain() const { return m_Swapchain != nullptr; }

        bool HasRenderer() const { return m_Renderer != nullptr; }

        Scene& GetScene() { return *m_Scene; }
        bool HasScene() const { return m_Scene != nullptr; }

        const Camera& GetEditorCamera() const;

        MeshLibrary& GetMeshLibrary();

        AssetDatabase& GetAssetDatabase() { return *m_AssetDatabase; }
        bool HasAssetDatabase() const { return m_AssetDatabase != nullptr; }

        AudioEngine& GetAudioEngine() { return *m_AudioEngine; }
        bool HasAudioEngine() const { return m_AudioEngine != nullptr; }

    private:
        bool m_Initialized = false;
        bool m_FlyMode = false;
        bool m_ViewportInteractive = false;

        std::unique_ptr<IPlatform> m_Platform;
        std::unique_ptr<GraphicsDevice> m_Device;
        std::unique_ptr<Swapchain> m_Swapchain;
        std::unique_ptr<Renderer> m_Renderer;
        std::unique_ptr<AudioEngine> m_AudioEngine;
        std::unique_ptr<AssetDatabase> m_AssetDatabase;
        std::unique_ptr<Scene> m_Scene;
        std::unique_ptr<EditorCamera> m_EditorCamera;

        ImGuiLayer m_ImGuiLayer;
    };
}