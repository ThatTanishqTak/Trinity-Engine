#include <Trinity/Core/Engine.h>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Assert.h>
#include <Trinity/Core/FileManagement.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Platform/PlatformFactory.h>
#include <Trinity/Renderer/RHI/GraphicsBackendFactory.h>
#include <Trinity/Renderer/RHI/Swapchain.h>

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
        l_SwapchainDescription.VSync = true;

        m_Swapchain = m_Device->CreateSwapchain(l_SwapchainDescription);
        if (m_Swapchain == nullptr)
        {
            TR_CORE_CRITICAL("Engine: swapchain creation failed");
            m_Device.reset();

            return false;
        }

        return true;
    }

    void Engine::Update(Timestep)
    {
        if (!m_Initialized)
        {
            return;
        }
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