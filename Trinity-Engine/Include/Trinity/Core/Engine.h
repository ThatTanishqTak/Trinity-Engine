#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <Trinity/Core/SimulationClock.h>
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
    class PhysicsSystem;
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
        void FixedUpdate(Timestep timestep);
        void Shutdown();

        bool StartScene();
        void StopScene();
        bool IsScenePlaying() const { return m_ScenePlaying; }

        // Unity's Pause freezes the fixed-rate simulation; Step releases exactly one tick.
        void SetScenePaused(bool paused) { m_ScenePaused = paused; }
        bool IsScenePaused() const { return m_ScenePaused; }
        void StepScene() { m_SceneStepRequested = true; }

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
        Renderer& GetRenderer();

        Scene& GetScene() { return *m_Scene; }
        bool HasScene() const { return m_Scene != nullptr; }

        const Camera& GetEditorCamera() const;
        EditorCamera& GetEditorCameraController();
        bool HasEditorCamera() const { return m_EditorCamera != nullptr; }

        MeshLibrary& GetMeshLibrary();

        AssetDatabase& GetAssetDatabase() { return *m_AssetDatabase; }
        bool HasAssetDatabase() const { return m_AssetDatabase != nullptr; }

        AudioEngine& GetAudioEngine() { return *m_AudioEngine; }
        bool HasAudioEngine() const { return m_AudioEngine != nullptr; }

        PhysicsSystem& GetPhysicsSystem() { return *m_PhysicsSystem; }
        bool HasPhysicsSystem() const { return m_PhysicsSystem != nullptr; }

        SimulationClock& GetSimulationClock() { return m_SimulationClock; }
        const SimulationClock& GetSimulationClock() const { return m_SimulationClock; }
        float GetInterpolationAlpha() const { return m_SimulationClock.GetAlpha(); }

    private:
        bool m_Initialized = false;
        bool m_FlyMode = false;
        bool m_ViewportInteractive = false;

        SimulationClock m_SimulationClock;

        bool m_ScenePlaying = false;
        bool m_ScenePaused = false;
        bool m_SceneStepRequested = false;
        std::string m_SceneSnapshot;

        std::unique_ptr<IPlatform> m_Platform;
        std::unique_ptr<GraphicsDevice> m_Device;
        std::unique_ptr<Swapchain> m_Swapchain;
        std::unique_ptr<Renderer> m_Renderer;
        std::unique_ptr<AudioEngine> m_AudioEngine;
        std::unique_ptr<PhysicsSystem> m_PhysicsSystem;
        std::unique_ptr<AssetDatabase> m_AssetDatabase;
        std::unique_ptr<Scene> m_Scene;
        std::unique_ptr<EditorCamera> m_EditorCamera;

        ImGuiLayer m_ImGuiLayer;
    };
}