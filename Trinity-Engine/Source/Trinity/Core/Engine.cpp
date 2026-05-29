#include <Trinity/Core/Engine.h>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Assert.h>
#include <Trinity/Core/FileManagement.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Platform/PlatformFactory.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>
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

#if defined(TRINITY_DEBUG)
        const bool l_EnableValidation = true;
#else
        const bool l_EnableValidation = false;
#endif

        m_Device = std::make_unique<VulkanDevice>();
        if (!m_Device->Initialize(window, applicationName, l_EnableValidation))
        {
            TR_CORE_CRITICAL("Engine: renderer initialization failed");
            m_Device.reset();

            return false;
        }

        SwapchainDescription l_SwapchainDesc;
        l_SwapchainDesc.Window = window;
        l_SwapchainDesc.Width = 1920;
        l_SwapchainDesc.Height = 1080;
        l_SwapchainDesc.VSync = true;

        if (!m_Device->CreateSwapchain(l_SwapchainDesc))
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
            m_Device->Shutdown();
            m_Device.reset();
        }

        if (m_Platform != nullptr)
        {
            m_Platform->Shutdown();
            m_Platform.reset();
        }

        m_Initialized = false;

        TR_CORE_INFO("Engine shut down");
    }
}