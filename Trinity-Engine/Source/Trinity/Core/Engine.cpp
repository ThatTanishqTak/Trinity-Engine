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
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>
#include <Trinity/Serialization/SceneSerializer.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    Engine::Engine() = default;

    Engine::~Engine()
    {
        if (m_Initialized)
        {
            ("Engine destroyed while still initialized");
        }
    }

    bool Engine::Initialize(const std::string& applicationName)
    {
        TR_CORE_ASSERT(!m_Initialized, "Engine already initialized");

        ("INITIALIZING ENGINE");

        m_Platform = PlatformFactory::Create();
        if (m_Platform == nullptr)
        {
            ("Failed to create platform");
            return false;
        }

        if (!m_Platform->Initialize())
        {
            ("Platform initialization failed");
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
            ("Audio engine initialization failed; continuing without audio");
        }

        m_Initialized = true;

        ("ENGINE INITIALIZED");

        return true;
    }

    bool Engine::InitializeRenderer(const NativeWindowHandle& window, const std::string& applicationName)
    {
        ("INITIALIZING RENDERER");

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
            ("Renderer initialization failed");
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
            ("Swapchain creation failed");
            m_Device.reset();

            return false;
        }

        m_Renderer = std::make_unique<Renderer>(*m_Device, *m_Swapchain, m_Platform->GetFileSystem());
        if (!m_Renderer->Initialize())
        {
            ("Renderer initialization failed");
            m_Renderer.reset();
            m_Swapchain.reset();
            m_Device.reset();

            return false;
        }

        m_AssetDatabase = std::make_unique<AssetDatabase>(m_Platform->GetFileSystem(), m_Renderer->GetMeshLibrary(), m_Renderer->GetTextureManager(), *m_AudioEngine);
        m_AssetDatabase->Initialize();

        m_EditorCamera = std::make_unique<EditorCamera>(60.0f, static_cast<float>(m_Swapchain->GetWidth()) / static_cast<float>(m_Swapchain->GetHeight()), 0.1f, 100.0f);

        Scene l_AuthoredScene;

        UUID l_TestMeshAsset = m_AssetDatabase->GetAssetByPath("Assets/Test.obj");

        Entity l_Parent = l_AuthoredScene.CreateEntity("ImportedMesh");
        l_Parent.AddComponent<MeshRendererComponent>(MeshRendererComponent{ nullptr, l_TestMeshAsset });
        l_Parent.GetComponent<TransformComponent>().Translation = glm::vec3(-1.5f, 0.0f, 0.0f);

        Entity l_Child = l_AuthoredScene.CreateEntity("ChildCube");
        l_Child.AddComponent<MeshRendererComponent>(MeshRendererComponent{ nullptr, UUID(AssetDatabase::BuiltinCube) });
        l_Child.GetComponent<TransformComponent>().Translation = glm::vec3(3.0f, 0.0f, 0.0f);
        l_Child.GetComponent<TransformComponent>().Scale = glm::vec3(0.5f, 0.5f, 0.5f);
        l_AuthoredScene.SetParent(l_Child, l_Parent);

        Entity l_CameraEntity = l_AuthoredScene.CreateEntity("PrimaryCamera");
        l_CameraEntity.AddComponent<CameraComponent>();

        std::filesystem::path l_ScenePath = m_Platform->GetFileSystem().Resolve(BaseDirectory::UserData, "Scenes/Demo.tscene");
        SceneSerializer::Serialize(l_AuthoredScene, l_ScenePath, "Demo");

        m_Scene = std::make_unique<Scene>();
        if (!SceneSerializer::Deserialize(*m_Scene, *m_AssetDatabase, l_ScenePath))
        {
            ("Failed to deserialize demo scene");
        }

        ("RENDERER INITIALIZED");

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
    }

    void Engine::InitializeImGui()
    {
        ("INITIALIZING IMGUI");

        if (m_Platform == nullptr || m_Device == nullptr || m_Swapchain == nullptr)
        {
            ("InitializeImGui called before renderer is ready");

            return;
        }

        m_ImGuiLayer.Initialize(m_Platform->GetImGuiBackend(), m_Device->GetImGuiBackend(), m_Swapchain->GetFramesInFlight(), m_Swapchain->GetFormat());
        
        ("IMGUI INITIALIZED");
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

    void Engine::Shutdown()
    {
        ("SHUTTING DOWN ENGINE");

        if (!m_Initialized)
        {
            return;
        }

        if (m_Device != nullptr)
        {
            m_Device->WaitIdle();
        }

        m_ImGuiLayer.Shutdown();

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

        ("ENGINE SHUTDOWN COMPLETE");
    }
}