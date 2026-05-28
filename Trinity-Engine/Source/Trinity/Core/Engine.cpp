#include <Trinity/Core/Engine.h>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Assert.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/PlatformFactory.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/FileManagement.h>

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

        TR_CORE_INFO("Engine initializing");

        m_Platform = PlatformFactory::Create();
        if (m_Platform == nullptr)
        {
            TR_CORE_CRITICAL("Engine: failed to create platform");
            return false;
        }

        if (!m_Platform->Initialize())
        {
            TR_CORE_CRITICAL("Engine: platform initialization failed");
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

        TR_CORE_INFO("Engine initialized");
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

        if (m_Platform != nullptr)
        {
            m_Platform->Shutdown();
            m_Platform.reset();
        }

        m_Initialized = false;

        TR_CORE_INFO("Engine shut down");
    }
}