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
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>
#include <Trinity/Serialization/SceneSerializer.h>

namespace Trinity
{
    Engine::Engine() = default;

    Engine::~Engine()
    {
        if (m_Initialized)
        {
            TR_CORE_WARN("Engine destroyed while still initialized");
        }
    }

    bool Engine::Initialize(const std::string& applicationName)
    {
        TR_CORE_ASSERT(!m_Initialized, "Engine already initialized");

        TR_CORE_INFO("[ENGINE]: INITIALIZING ENGINE");

        m_Platform = PlatformFactory::Create();
        if (m_Platform == nullptr)
        {
            TR_CORE_CRITICAL("Engine: failed to create platform");
            return false;
        }

        if (!m_Platform->Initialize())
        {
            TR_CORE_CRITICAL("[Engine]: platform initialization failed");
            m_Platform.reset();

            return false;
        }

        FileSystem& l_FileSystem = m_Platform->GetFileSystem();
        l_FileSystem.SetApplicationName(applicationName);

        std::filesystem::path l_LogDirectory = l_FileSystem.GetBaseDirectory(BaseDirectory::UserData);
        FileManagement::EnsureDirectory(l_LogDirectory);

        std::filesystem::path l_LogPath = l_FileSystem.Resolve(BaseDirectory::UserData, applicationName + ".log");
        Log::InitializeFileSink(l_LogPath);

        m_Initialized = true;

        TR_CORE_INFO("ENGINE INITIALIZED");
        return true;
    }

    bool Engine::InitializeRenderer(const NativeWindowHandle& window, const std::string& applicationName)
    {
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
            TR_CORE_CRITICAL("Engine: renderer initialization failed");
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
            TR_CORE_CRITICAL("Engine: swapchain creation failed");
            m_Device.reset();

            return false;
        }

        m_Renderer = std::make_unique<Renderer>(*m_Device, *m_Swapchain, m_Platform->GetFileSystem());
        if (!m_Renderer->Initialize())
        {
            TR_CORE_CRITICAL("Engine: renderer initialization failed");
            m_Renderer.reset();
            m_Swapchain.reset();
            m_Device.reset();

            return false;
        }

        m_EditorCamera = std::make_unique<EditorCamera>(60.0f, static_cast<float>(m_Swapchain->GetWidth()) / static_cast<float>(m_Swapchain->GetHeight()), 0.1f, 100.0f);

        MeshLibrary& l_MeshLibrary = m_Renderer->GetMeshLibrary();

        Scene l_AuthoredScene;

        Entity l_Parent = l_AuthoredScene.CreateEntity("ImportedMesh");
        l_Parent.AddComponent<MeshRendererComponent>(MeshRendererComponent{ nullptr, "Assets/Test.obj" });
        l_Parent.GetComponent<TransformComponent>().Translation = glm::vec3(-1.5f, 0.0f, 0.0f);

        Entity l_Child = l_AuthoredScene.CreateEntity("ChildCube");
        l_Child.AddComponent<MeshRendererComponent>(MeshRendererComponent{ nullptr, "" });
        l_Child.GetComponent<TransformComponent>().Translation = glm::vec3(3.0f, 0.0f, 0.0f);
        l_Child.GetComponent<TransformComponent>().Scale = glm::vec3(0.5f, 0.5f, 0.5f);
        l_AuthoredScene.SetParent(l_Child, l_Parent);

        Entity l_CameraEntity = l_AuthoredScene.CreateEntity("PrimaryCamera");
        l_CameraEntity.AddComponent<CameraComponent>();

        std::filesystem::path l_ScenePath = m_Platform->GetFileSystem().Resolve(BaseDirectory::UserData, "Scenes/Demo.tscene");
        SceneSerializer::Serialize(l_AuthoredScene, l_ScenePath, "Demo");

        m_Scene = std::make_unique<Scene>();
        if (!SceneSerializer::Deserialize(*m_Scene, l_MeshLibrary, l_ScenePath))
        {
            TR_CORE_ERROR("Engine: failed to deserialize demo scene");
        }

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
    }

    void Engine::InitializeImGui()
    {
        if (m_Platform == nullptr || m_Device == nullptr || m_Swapchain == nullptr)
        {
            TR_CORE_ERROR("Engine: InitializeImGui called before renderer is ready");

            return;
        }

        m_ImGuiLayer.Initialize(m_Platform->GetImGuiBackend(), m_Device->GetImGuiBackend(), m_Swapchain->GetFramesInFlight(), m_Swapchain->GetFormat());
    }

    void Engine::BeginImGuiFrame()
    {
        m_ImGuiLayer.BeginFrame();
    }

    void Engine::RenderFrame()
    {
        if (m_Renderer != nullptr && m_Scene != nullptr && m_EditorCamera != nullptr)
        {
            m_Renderer->RenderFrame(*m_Scene, m_EditorCamera->GetCamera(), &m_ImGuiLayer);
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

    void Engine::SetViewportInteractive(bool interactive)
    {
        m_ViewportInteractive = interactive;
    }

    void Engine::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        TR_CORE_INFO("Engine shutting down");

        if (m_Device != nullptr)
        {
            m_Device->WaitIdle();
        }

        m_ImGuiLayer.Shutdown();

        m_Scene.reset();
        m_EditorCamera.reset();

        if (m_Renderer != nullptr)
        {
            m_Renderer.reset();
        }

        if (m_Swapchain != nullptr)
        {
            m_Swapchain.reset();
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

        TR_CORE_INFO("Engine shut down");
    }
}