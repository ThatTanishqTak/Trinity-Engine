#include <Trinity/ImGui/ImGuiLayer.h>

#include <imgui.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    ImGuiLayer::~ImGuiLayer()
    {
        Shutdown();
    }

    bool ImGuiLayer::Initialize(IImGuiPlatformBackend& platform, IImGuiRenderBackend& renderer, uint32_t framesInFlight, Format colorFormat)
    {
        TR_CORE_TRACE("INITIALIZING IMGUI LAYER");

        if (m_Initialized)
        {
            return true;
        }

        m_Platform = &platform;
        m_Render = &renderer;

        TR_CORE_TRACE("ImGui version: {}", IMGUI_VERSION);
        TR_CORE_TRACE("ImGui version and data layout match: {}", IMGUI_CHECKVERSION());

        ImGui::CreateContext();

        ImGuiIO& l_IO = ImGui::GetIO();
        l_IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        l_IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        if (!m_Platform->Initialize())
        {
            TR_CORE_CRITICAL("Failed to initialize platform");
            ImGui::DestroyContext();
            m_Platform = nullptr;
            m_Render = nullptr;

            return false;
        }

        if (!m_Render->Initialize(framesInFlight, colorFormat))
        {
            TR_CORE_CRITICAL("Failed to initialize renderer");
            m_Platform->Shutdown();
            ImGui::DestroyContext();
            m_Platform = nullptr;
            m_Render = nullptr;

            return false;
        }

        m_Initialized = true;

        TR_CORE_TRACE("IMGUI LAYER INITIALIZED");

        return true;
    }

    void ImGuiLayer::Shutdown()
    {
        TR_CORE_TRACE("SHUTTING DOWN IMGUI LAYER");

        if (!m_Initialized)
        {
            return;
        }

        if (m_Render != nullptr)
        {
            m_Render->Shutdown();
        }

        if (m_Platform != nullptr)
        {
            m_Platform->Shutdown();
        }

        ImGui::DestroyContext();

        m_Render = nullptr;
        m_Platform = nullptr;
        m_Initialized = false;

        TR_CORE_TRACE("IMGUI LAYER SHUTDOWN COMPLETE");
    }

    void ImGuiLayer::BeginFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        m_Render->NewFrame();
        m_Platform->NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::EndFrame(CommandList& commandList)
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui::Render();
        m_Render->RecordDrawData(commandList);
    }
}