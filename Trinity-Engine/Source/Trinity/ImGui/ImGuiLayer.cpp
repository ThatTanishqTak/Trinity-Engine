#include <Trinity/ImGui/ImGuiLayer.h>

#include <imgui.h>

#include <cmath>

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

        if (colorFormat == Format::BGRA8_SRGB)
        {
            ImGuiStyle& l_Style = ImGui::GetStyle();
            for (int l_Index = 0; l_Index < ImGuiCol_COUNT; ++l_Index)
            {
                ImVec4& l_Color = l_Style.Colors[l_Index];
                l_Color.x = std::pow(l_Color.x, 2.2f);
                l_Color.y = std::pow(l_Color.y, 2.2f);
                l_Color.z = std::pow(l_Color.z, 2.2f);
            }
        }

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