#include <Trinity/Core/Engine.h>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Assert.h>
#include <Trinity/Core/FileManagement.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Platform/PlatformFactory.h>
#include <Trinity/Renderer/RHI/GraphicsBackendFactory.h>
#include <Trinity/Renderer/RHI/Swapchain.h>
#include <Trinity/Renderer/Frontend/Renderer.h>
#include <Trinity/Renderer/Frontend/EditorCamera.h>
#include <Trinity/Audio/Frontend/AudioEngine.h>
#include <Trinity/Physics/Frontend/PhysicsSystem.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>
#include <Trinity/Scene/Components/BoxCollider2DComponent.h>
#include <Trinity/Scene/Components/Rigidbody2DComponent.h>
#include <Trinity/Serialization/SceneSerializer.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    Engine::Engine() = default;

    Engine::~Engine()
    {

    }

    bool Engine::Initialize(const std::string& applicationName)
    {
        TR_CORE_INFO("INITIALIZING ENGINE");

        m_Platform = PlatformFactory::Create();
        if (m_Platform == nullptr)
        {
            TR_CORE_CRITICAL("Failed to create platform");
            return false;
        }

        if (!m_Platform->Initialize())
        {
            TR_CORE_CRITICAL("Failed to initialize platform");
            m_Platform.reset();

            return false;
        }

        FileSystem& l_FileSystem = m_Platform->GetFileSystem();
        l_FileSystem.SetApplicationName(applicationName);

        std::filesystem::path l_LogDirectory = l_FileSystem.GetBaseDirectory(BaseDirectory::UserData);
        FileManagement::EnsureDirectory(l_LogDirectory);

        std::filesystem::path l_LogPath = l_FileSystem.Resolve(BaseDirectory::UserData, applicationName + ".log");
        Log::InitializeFileSink(l_LogPath);

        m_AudioEngine = std::make_unique<AudioEngine>();
        if (!m_AudioEngine->Initialize())
        {
            TR_CORE_CRITICAL("Failed to initialize audio engine");
        }

        m_PhysicsSystem = std::make_unique<PhysicsSystem>();
        if (!m_PhysicsSystem->Initialize())
        {
            TR_CORE_CRITICAL("Failed to initialize physics system");
            m_PhysicsSystem.reset();
        }

        m_Initialized = true;

        TR_CORE_INFO("ENGINE INITIALIZED");

        return true;
    }

    bool Engine::InitializeRenderer(const NativeWindowHandle& window, const std::string& applicationName)
    {
        TR_CORE_INFO("INITIALIZING RENDERER");

        TR_CORE_ASSERT(m_Initialized, "Engine must be initialized before renderer");
        TR_CORE_ASSERT(m_Device == nullptr, "Renderer already initialized");

        GraphicsDeviceDescription l_DeviceDescription;
        l_DeviceDescription.Window = window;
        l_DeviceDescription.ApplicationName = applicationName;

#if defined(TRINITY_DEBUG)
        l_DeviceDescription.EnableValidation = true;
#else
        l_DeviceDescription.EnableValidation = false;
#endif

        m_Device = GraphicsBackendFactory::Create(l_DeviceDescription);
        if (m_Device == nullptr)
        {
            TR_CORE_CRITICAL("Failed to create logical device");
            return false;
        }

        SwapchainDescription l_SwapchainDescription;
        l_SwapchainDescription.Window = window;
        l_SwapchainDescription.Width = 1920;
        l_SwapchainDescription.Height = 1080;
        l_SwapchainDescription.PreferredFormat = Format::BGRA8_UNORM;
        l_SwapchainDescription.VSync = true;

        m_Swapchain = m_Device->CreateSwapchain(l_SwapchainDescription);
        if (m_Swapchain == nullptr)
        {
            TR_CORE_CRITICAL("Failed to create swapchain");
            m_Device.reset();

            return false;
        }

        m_Renderer = std::make_unique<Renderer>(*m_Device, *m_Swapchain, m_Platform->GetFileSystem());
        if (!m_Renderer->Initialize())
        {
            TR_CORE_CRITICAL("Failed to create renderer");
            m_Renderer.reset();
            m_Swapchain.reset();
            m_Device.reset();

            return false;
        }

        m_AssetDatabase = std::make_unique<AssetDatabase>(m_Platform->GetFileSystem(), m_Renderer->GetMeshLibrary(), m_Renderer->GetTextureManager(), *m_AudioEngine);
        m_AssetDatabase->Initialize();

        m_EditorCamera = std::make_unique<EditorCamera>(60.0f, static_cast<float>(m_Swapchain->GetWidth()) / static_cast<float>(m_Swapchain->GetHeight()), 0.1f, 100.0f);

        Scene l_AuthoredScene;

        Entity l_Ground = l_AuthoredScene.CreateEntity("Ground");
        l_Ground.AddComponent<MeshRendererComponent>(MeshRendererComponent{ nullptr, UUID(AssetDatabase::BuiltinQuad) });
        l_Ground.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, -2.0f, 0.0f);
        l_Ground.GetComponent<TransformComponent>().Scale = glm::vec3(20.0f, 1.0f, 1.0f);
        Rigidbody2DComponent l_GroundBody;
        l_GroundBody.Type = BodyType::Static;
        l_Ground.AddComponent<Rigidbody2DComponent>(l_GroundBody);
        l_Ground.AddComponent<BoxCollider2DComponent>();

        Entity l_Box = l_AuthoredScene.CreateEntity("FallingBox");
        l_Box.AddComponent<MeshRendererComponent>(MeshRendererComponent{ nullptr, UUID(AssetDatabase::BuiltinQuad) });
        l_Box.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, 5.0f, 0.0f);
        l_Box.AddComponent<Rigidbody2DComponent>();
        l_Box.AddComponent<BoxCollider2DComponent>();

        Entity l_CameraEntity = l_AuthoredScene.CreateEntity("PrimaryCamera");
        l_CameraEntity.AddComponent<CameraComponent>();

        std::filesystem::path l_ScenePath = m_Platform->GetFileSystem().Resolve(BaseDirectory::UserData, "Scenes/Demo.tscene");
        SceneSerializer::Serialize(l_AuthoredScene, l_ScenePath, "Demo");

        m_Scene = std::make_unique<Scene>();
        if (!SceneSerializer::Deserialize(*m_Scene, *m_AssetDatabase, l_ScenePath))
        {
            TR_CORE_WARN("Failed to deserialize scene");
        }

        TR_CORE_INFO("RENDERER INITIALIZED");

        return true;
    }

    void Engine::Update(Timestep timestep)
    {
        if (!m_Initialized)
        {
            return;
        }

        if (m_EditorCamera != nullptr && m_Platform != nullptr)
        {
            Input& l_Input = m_Platform->GetInput();
            bool l_RightDown = l_Input.IsMouseButtonPressed(Mouse::ButtonRight);

            if (!m_FlyMode && l_RightDown && m_ViewportInteractive)
            {
                m_FlyMode = true;
                if (m_Platform->HasWindow())
                {
                    m_Platform->GetWindow().SetRelativeMouseMode(true);
                }
            }
            else if (m_FlyMode && !l_RightDown)
            {
                m_FlyMode = false;
                if (m_Platform->HasWindow())
                {
                    m_Platform->GetWindow().SetRelativeMouseMode(false);
                }
            }

            m_EditorCamera->OnUpdate(l_Input, timestep, m_FlyMode);
        }

        if (m_Renderer != nullptr)
        {
            m_Renderer->ApplyViewportResize();
        }

        if (m_AudioEngine != nullptr && m_Scene != nullptr && m_AssetDatabase != nullptr)
        {
            m_AudioEngine->Update(*m_Scene, *m_AssetDatabase);
        }

        if (m_PhysicsSystem != nullptr && m_Scene != nullptr && m_ScenePlaying)
        {
            m_PhysicsSystem->ApplyInterpolation(*m_Scene, m_SimulationClock.GetAlpha());
        }
    }

    void Engine::FixedUpdate(Timestep timestep)
    {
        if (!m_Initialized)
        {
            return;
        }

        // Fixed-rate simulation only; input, cameras, and audio stay on the variable-rate Update path
        if (m_PhysicsSystem != nullptr && m_Scene != nullptr && m_ScenePlaying)
        {
            m_PhysicsSystem->Step(*m_Scene, timestep.GetSeconds());
        }
    }

    void Engine::InitializeImGui()
    {
        TR_CORE_INFO("INITIALIZING IMGUI");

        if (m_Platform == nullptr || m_Device == nullptr || m_Swapchain == nullptr)
        {
            return;
        }

        m_ImGuiLayer.Initialize(m_Platform->GetImGuiBackend(), m_Device->GetImGuiBackend(), m_Swapchain->GetFramesInFlight(), m_Swapchain->GetFormat());

        TR_CORE_INFO("IMGUI INITIALIZED");
    }

    void Engine::BeginImGuiFrame()
    {
        m_ImGuiLayer.BeginFrame();
    }

    void Engine::RenderFrame()
    {
        if (m_Renderer != nullptr && m_Scene != nullptr && m_EditorCamera != nullptr && m_AssetDatabase != nullptr)
        {
            m_Renderer->RenderFrame(*m_Scene, *m_AssetDatabase, m_EditorCamera->GetCamera(), &m_ImGuiLayer);
        }

        // Physics events stay queued for the whole frame so panels can read them; the frame ends here
        if (m_PhysicsSystem != nullptr)
        {
            m_PhysicsSystem->ClearEvents();
        }
    }

    void Engine::Resize(uint32_t width, uint32_t height)
    {
        if (m_Renderer != nullptr)
        {
            m_Renderer->Resize(width, height);
        }

        if (m_EditorCamera != nullptr && width > 0 && height > 0)
        {
            m_EditorCamera->SetViewportSize(static_cast<float>(width), static_cast<float>(height));
        }
    }

    void Engine::SetViewportSize(uint32_t width, uint32_t height)
    {
        if (m_Renderer != nullptr)
        {
            m_Renderer->SetViewportSize(width, height);
        }

        if (m_EditorCamera != nullptr && width > 0 && height > 0)
        {
            m_EditorCamera->SetViewportSize(static_cast<float>(width), static_cast<float>(height));
        }
    }

    uint64_t Engine::GetViewportTextureID() const
    {
        return m_Renderer != nullptr ? m_Renderer->GetViewportTextureID() : 0;
    }

    const Camera& Engine::GetEditorCamera() const
    {
        return m_EditorCamera->GetCamera();
    }

    EditorCamera& Engine::GetEditorCameraController()
    {
        return *m_EditorCamera;
    }

    MeshLibrary& Engine::GetMeshLibrary()
    {
        return m_Renderer->GetMeshLibrary();
    }

    Renderer& Engine::GetRenderer()
    {
        return *m_Renderer;
    }

    void Engine::SetViewportInteractive(bool interactive)
    {
        m_ViewportInteractive = interactive;
    }

    bool Engine::StartScene()
    {
        if (m_ScenePlaying)
        {
            return true;
        }

        if (m_Scene == nullptr || m_AssetDatabase == nullptr)
        {
            TR_CORE_WARN("Cannot enter play mode without a scene and an asset database");

            return false;
        }

        m_SceneSnapshot = SceneSerializer::SerializeToString(*m_Scene, "PlayModeSnapshot");
        if (m_SceneSnapshot.empty())
        {
            TR_CORE_ERROR("Failed to snapshot scene; refusing to enter play mode");

            return false;
        }

        m_SimulationClock.Reset();

        if (m_AudioEngine != nullptr)
        {
            m_AudioEngine->StartScene(*m_Scene, *m_AssetDatabase);
        }

        if (m_PhysicsSystem != nullptr)
        {
            m_PhysicsSystem->StartScene(*m_Scene);
        }

        m_ScenePlaying = true;

        return true;
    }

    void Engine::StopScene()
    {
        if (!m_ScenePlaying)
        {
            return;
        }

        if (m_AudioEngine != nullptr && m_Scene != nullptr)
        {
            m_AudioEngine->StopScene(*m_Scene);
        }

        if (m_PhysicsSystem != nullptr && m_Scene != nullptr)
        {
            m_PhysicsSystem->StopScene(*m_Scene);
        }

        if (m_Scene != nullptr && m_AssetDatabase != nullptr && !m_SceneSnapshot.empty())
        {
            m_Scene->Clear();
            if (!SceneSerializer::DeserializeFromString(*m_Scene, *m_AssetDatabase, m_SceneSnapshot))
            {
                TR_CORE_ERROR("Failed to restore scene snapshot after play mode");
            }
        }

        m_SceneSnapshot.clear();
        m_ScenePlaying = false;
    }

    void Engine::Shutdown()
    {
        TR_CORE_INFO("SHUTTING DOWN ENGINE");

        if (!m_Initialized)
        {
            return;
        }

        if (m_Device != nullptr)
        {
            m_Device->WaitIdle();
        }

        m_ImGuiLayer.Shutdown();

        if (m_PhysicsSystem != nullptr)
        {
            m_PhysicsSystem->Shutdown();
            m_PhysicsSystem.reset();
        }

        m_Scene.reset();
        m_EditorCamera.reset();
        m_AssetDatabase.reset();
        m_AudioEngine.reset();

        if (m_Renderer != nullptr)
        {
            m_Renderer.reset();
        }

        if (m_Swapchain != nullptr)
        {
            m_Swapchain.reset();
        }

        if (m_Device != nullptr)
        {
            m_Device->Shutdown();
            m_Device.reset();
        }

        m_Initialized = false;

        TR_CORE_INFO("ENGINE SHUTDOWN COMPLETE");
    }
}