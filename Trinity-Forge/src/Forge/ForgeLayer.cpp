#include "Forge/ForgeLayer.h"

#include "Trinity/Application/Application.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Scene/SceneSerializer.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/Platform/Input/Desktop/DesktopInput.h"
#include "Trinity/Platform/Input/Desktop/DesktopInputCodes.h"
#include "Trinity/Version.h"

#include <imgui.h>
#include <ImGuizmo.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

namespace
{
    static std::string PlatformOpenFileDialog(const char* filter)
    {
#ifdef _WIN32
        OPENFILENAMEA l_OpenFile{};
        char l_Path[512] = {};
        l_OpenFile.lStructSize = sizeof(l_OpenFile);
        l_OpenFile.lpstrFilter = filter;
        l_OpenFile.lpstrFile = l_Path;
        l_OpenFile.nMaxFile = sizeof(l_Path);
        l_OpenFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&l_OpenFile))
        {
            return l_Path;
        }
#endif
        return {};
    }

    static std::string PlatformSaveFileDialog(const char* filter, const char* defaultExt)
    {
#ifdef _WIN32
        OPENFILENAMEA l_OpenFile{};
        char l_Path[512] = {};
        l_OpenFile.lStructSize = sizeof(l_OpenFile);
        l_OpenFile.lpstrFilter = filter;
        l_OpenFile.lpstrFile = l_Path;
        l_OpenFile.nMaxFile = sizeof(l_Path);
        l_OpenFile.lpstrDefExt = defaultExt;
        l_OpenFile.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&l_OpenFile))
        {
            return l_Path;
        }
#endif
        return {};
    }
}

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{

}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    m_SelectionContext.ActiveScene = &m_Scene;

    m_PanelManager.Initialize();

    m_ViewportPanel = m_PanelManager.RegisterPanel<Forge::ViewportPanel>("Viewport", &m_SelectionContext);
    m_HierarchyPanel = m_PanelManager.RegisterPanel<Forge::SceneHierarchyPanel>("Scene Hierarchy", &m_SelectionContext);
    m_InspectorPanel = m_PanelManager.RegisterPanel<Forge::InspectorPanel>("Inspector", &m_SelectionContext);
    m_ContentPanel = m_PanelManager.RegisterPanel<Forge::ContentBrowserPanel>("Content Browser");
    m_StatsPanel = m_PanelManager.RegisterPanel<Forge::RendererStatsPanel>("Renderer Stats");
    m_LogPanel = m_PanelManager.RegisterPanel<Forge::LogPanel>("Log");

    Trinity::ImGuiLayer::Get().SetMenuBarCallback([this]()
    {
        RenderMenuBar();
    });

    Trinity::ImGuiLayer::Get().SetTitleBarCallback([this]()
    {
        RenderTitleBar();
    });

    UpdateWindowTitle();
}

void ForgeLayer::OnShutdown()
{
    m_PanelManager.Shutdown();
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    // Keyboard shortcuts — blocked during play/pause
    const bool l_Ctrl = Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_LEFT_CONTROL);
    const bool l_Shift = Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_LEFT_SHIFT);

    if (l_Ctrl && m_SelectionContext.State == EditorState::Edit)
    {
        if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_N))
        {
            NewScene();
        }
        else if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_O))
        {
            const std::string l_Path = PlatformOpenFileDialog("Trinity Scene\0*.tscene\0All Files\0*.*\0");
            if (!l_Path.empty())
            {
                OpenScene(l_Path);
            }
        }
        else if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_S))
        {
            if (l_Shift || m_CurrentScenePath.empty())
            {
                const std::string l_Path = PlatformSaveFileDialog("Trinity Scene\0*.tscene\0All Files\0*.*\0", "tscene");
                if (!l_Path.empty())
                {
                    SaveSceneAs(l_Path);
                }
            }
            else
            {
                SaveScene();
            }
        }
    }

    // Detect state transitions and react
    if (m_SelectionContext.State != m_LastEditorState)
    {
        const EditorState l_Previous = m_LastEditorState;
        const EditorState l_Current = m_SelectionContext.State;

        if (l_Current == EditorState::Play && l_Previous == EditorState::Edit)
        {
            m_SceneSnapshot = Trinity::SceneSerializer::SerializeToString(m_Scene);
        }
        else if (l_Current == EditorState::Edit && (l_Previous == EditorState::Play || l_Previous == EditorState::Pause))
        {
            if (!m_SceneSnapshot.empty())
            {
                m_Scene = Trinity::Scene();
                Trinity::SceneSerializer::DeserializeFromString(m_Scene, m_SceneSnapshot);
                m_SelectionContext.ActiveScene = &m_Scene;
            }
            m_SelectionContext.SelectedEntity = entt::null;
        }

        m_LastEditorState = l_Current;
    }

    const float l_EffectiveDelta = (m_SelectionContext.State == EditorState::Pause) ? 0.0f : deltaTime;
    m_PanelManager.UpdatePanels(l_EffectiveDelta);
}

void ForgeLayer::OnRender()
{
    m_PanelManager.PreRenderPanels();
}

void ForgeLayer::OnImGuiRender()
{
    ImGuizmo::BeginFrame();

    m_PanelManager.RenderPanels();

    if (m_ShowAboutPopup)
    {
        ImGui::OpenPopup("About Trinity Engine");
        m_ShowAboutPopup = false;
    }

    ImVec2 l_Center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(l_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("About Trinity Engine", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Trinity Engine");
        ImGui::Separator();
        ImGui::Text("Vulkan-backed game engine with deferred rendering.");
        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    (void)e;
}

void ForgeLayer::NewScene()
{
    m_Scene = Trinity::Scene("Untitled");
    m_CurrentScenePath.clear();
    m_SceneSnapshot.clear();
    m_SelectionContext.SelectedEntity = entt::null;
    m_SelectionContext.ActiveScene = &m_Scene;

    UpdateWindowTitle();
}

void ForgeLayer::OpenScene(const std::string& filepath)
{
    Trinity::Scene l_NewScene;
    if (Trinity::SceneSerializer::Deserialize(l_NewScene, filepath))
    {
        m_Scene = std::move(l_NewScene);
        m_CurrentScenePath = filepath;
        m_SceneSnapshot.clear();
        m_SelectionContext.SelectedEntity = entt::null;
        m_SelectionContext.ActiveScene = &m_Scene;

        UpdateWindowTitle();
    }
}

void ForgeLayer::SaveScene()
{
    if (!m_CurrentScenePath.empty())
    {
        Trinity::SceneSerializer::Serialize(m_Scene, m_CurrentScenePath);
    }
}

void ForgeLayer::SaveSceneAs(const std::string& filepath)
{
    Trinity::SceneSerializer::Serialize(m_Scene, filepath);
    m_CurrentScenePath = filepath;
}

void ForgeLayer::RenderTitleBar()
{
    const float l_Width = ImGui::GetWindowWidth();
    const float l_Height = ImGui::GetWindowHeight();
    const float l_TextOffsetY = (l_Height - ImGui::GetTextLineHeight()) * 0.5f;

    ImGui::SetCursorPos(ImVec2(8.0f, l_TextOffsetY));
    ImGui::TextColored(ImVec4(0.47f, 0.77f, 0.47f, 1.0f), "Trinity Forge");

    const std::string& l_SceneName = m_Scene.GetName();
    const float l_SceneNameWidth = ImGui::CalcTextSize(l_SceneName.c_str()).x;
    ImGui::SetCursorPos(ImVec2((l_Width - l_SceneNameWidth) * 0.5f, l_TextOffsetY));
    ImGui::Text("%s", l_SceneName.c_str());

    const char* l_APIName = Trinity::Renderer::GetBackend() == Trinity::RendererBackend::Vulkan ? "Vulkan" : "Unknown";
    const std::string l_RightText = std::string(l_APIName) + "  |  v" + Trinity::EngineVersion;
    const float l_RightTextWidth = ImGui::CalcTextSize(l_RightText.c_str()).x;
    ImGui::SetCursorPos(ImVec2(l_Width - l_RightTextWidth - 8.0f, l_TextOffsetY));
    ImGui::TextDisabled("%s", l_RightText.c_str());
}

void ForgeLayer::UpdateWindowTitle()
{
    const char* l_APIName = Trinity::Renderer::GetBackend() == Trinity::RendererBackend::Vulkan ? "Vulkan" : "Unknown";
    const std::string l_Title = std::string("Trinity Forge  |  ") + m_Scene.GetName() + "  |  " + l_APIName + "  |  v" + Trinity::EngineVersion;
    Trinity::Application::Get().GetWindow().SetTitle(l_Title);
}

void ForgeLayer::RenderMenuBar()
{
    // File
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Scene", "Ctrl+N"))
        {
            NewScene();
        }

        if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
        {
            const std::string l_Path = PlatformOpenFileDialog("Trinity Scene\0*.tscene\0All Files\0*.*\0");
            if (!l_Path.empty())
            {
                OpenScene(l_Path);
            }
        }

        ImGui::Separator();

        const bool l_HasPath = !m_CurrentScenePath.empty();

        if (!l_HasPath)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            SaveScene();
        }

        if (!l_HasPath)
        {
            ImGui::EndDisabled();
        }

        if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
        {
            const std::string l_Path = PlatformSaveFileDialog("Trinity Scene\0*.tscene\0All Files\0*.*\0", "tscene");
            if (!l_Path.empty())
            {
                SaveSceneAs(l_Path);
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
        {
            Trinity::Application::Get().Close();
        }

        ImGui::EndMenu();
    }

    // Edit
    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Undo", "Ctrl+Z");
        ImGui::MenuItem("Redo", "Ctrl+Y");
        ImGui::EndDisabled();

        ImGui::EndMenu();
    }

    // View
    if (ImGui::BeginMenu("View"))
    {
        m_PanelManager.RenderViewMenu();
        ImGui::EndMenu();
    }

    // Renderer
    if (ImGui::BeginMenu("Renderer"))
    {
        ImGui::Text("Backend: %s", Trinity::Renderer::GetBackend() == Trinity::RendererBackend::Vulkan ? "Vulkan" : "Unknown");

        ImGui::Separator();

        ImGui::BeginDisabled();
        ImGui::MenuItem("Show GeometryBuffer Albedo");
        ImGui::MenuItem("Show GeometryBuffer Normal");
        ImGui::MenuItem("Show GeometryBuffer MRA");
        ImGui::MenuItem("Show Shadow Map");
        ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::MenuItem("Renderer Stats", nullptr, &m_StatsPanel->GetOpenState());

        ImGui::EndMenu();
    }

    // Help
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("About Trinity Engine"))
        {
            m_ShowAboutPopup = true;
        }

        ImGui::EndMenu();
    }
}
