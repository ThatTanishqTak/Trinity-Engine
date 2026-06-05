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
        if (m_Initialized)
        {
            return true;
        }

        m_Platform = &platform;
        m_Render = &renderer;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& l_IO = ImGui::GetIO();
        l_IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        l_IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        if (!m_Platform->Initialize())
        {
            TR_CORE_ERROR("ImGuiLayer: platform backend init failed");
            ImGui::DestroyContext();
            m_Platform = nullptr;
            m_Render = nullptr;

            return false;
        }

        if (!m_Render->Initialize(framesInFlight, colorFormat))
        {
            TR_CORE_ERROR("ImGuiLayer: render backend init failed");
            m_Platform->Shutdown();
            ImGui::DestroyContext();
            m_Platform = nullptr;
            m_Render = nullptr;

            return false;
        }

        m_Initialized = true;
        TR_CORE_INFO("ImGuiLayer: initialized");

        return true;
    }

    void ImGuiLayer::Shutdown()
    {
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

    void ImGuiLayer::EndFrame(CommandList& a_CommandList)
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui::Render();
        m_Render->RecordDrawData(a_CommandList);
    }
}